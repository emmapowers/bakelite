#!/usr/bin/env python3
"""Run benchmarks across architectures and optimization levels."""

import json
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from datetime import datetime, timezone
from enum import StrEnum, auto
from pathlib import Path

from download_toolchains import get_or_download_toolchain

BENCH_DIR = Path(__file__).parent.parent
PROTOCOLS_DIR = BENCH_DIR / "protocols"
HARNESS_DIR = BENCH_DIR / "harness"
RESULTS_DIR = BENCH_DIR / "results"


class Language(StrEnum):
    CTINY = auto()
    CPPTINY = auto()


class Target(StrEnum):
    ARM_CORTEX_M0 = "arm-cortex-m0"
    ARM_CORTEX_M3 = "arm-cortex-m3"
    AVR_ATMEGA328P = "avr-atmega328p"
    RISCV32_IMC = "riscv32-imc"
    NATIVE_X64 = "native-x64"


class OptLevel(StrEnum):
    O0 = "-O0"
    Os = "-Os"
    O2 = "-O2"
    O3 = "-O3"


@dataclass
class TargetConfig:
    toolchain: str
    compiler: str
    flags: list[str]
    size_cmd: str
    objdump_cmd: str
    unpacked: bool = False  # Use --unpacked for platforms without unaligned access


TARGET_CONFIGS: dict[Target, TargetConfig] = {
    Target.ARM_CORTEX_M0: TargetConfig(
        toolchain="arm-none-eabi",
        compiler="arm-none-eabi-gcc",
        flags=["-mcpu=cortex-m0", "-mthumb", "-mfloat-abi=soft", "-ffreestanding"],
        size_cmd="arm-none-eabi-size",
        objdump_cmd="arm-none-eabi-objdump",
        unpacked=True,  # M0 doesn't support unaligned access
    ),
    Target.ARM_CORTEX_M3: TargetConfig(
        toolchain="arm-none-eabi",
        compiler="arm-none-eabi-gcc",
        flags=["-mcpu=cortex-m3", "-mthumb", "-mfloat-abi=soft", "-ffreestanding"],
        size_cmd="arm-none-eabi-size",
        objdump_cmd="arm-none-eabi-objdump",
    ),
    Target.AVR_ATMEGA328P: TargetConfig(
        toolchain="avr",
        compiler="avr-gcc",
        flags=["-mmcu=atmega328p", "-ffreestanding"],
        size_cmd="avr-size",
        objdump_cmd="avr-objdump",
        unpacked=True,  # AVR doesn't support unaligned access
    ),
    Target.RISCV32_IMC: TargetConfig(
        toolchain="riscv-none-elf",
        compiler="riscv-none-elf-gcc",
        flags=["-march=rv32imc", "-mabi=ilp32", "-ffreestanding"],
        size_cmd="riscv-none-elf-size",
        objdump_cmd="riscv-none-elf-objdump",
        unpacked=True,  # RISC-V base ISA doesn't guarantee unaligned access
    ),
    Target.NATIVE_X64: TargetConfig(
        toolchain="native",
        compiler="cc",
        flags=["-arch", "x86_64"],
        size_cmd="size",
        objdump_cmd="objdump",
    ),
}


@dataclass
class BenchConfig:
    protocol: str
    language: Language
    target: Target
    opt_level: OptLevel


@dataclass
class BenchResult:
    config: BenchConfig
    text_size: int
    data_size: int
    bss_size: int
    instruction_count: int
    stack_usage: dict[str, int] = field(default_factory=dict)


def run_cmd(args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    """Run command and return result."""
    result = subprocess.run(args, check=False, capture_output=True, text=True, cwd=cwd)
    if result.returncode != 0:
        print(f"Command failed: {' '.join(args)}")
        print(f"stderr: {result.stderr}")
        raise RuntimeError(f"Command failed: {args[0]}")
    return result


def generate_code(
    protocol: Path, language: Language, output_dir: Path, *, unpacked: bool = False
) -> Path:
    """Generate protocol code using bakelite."""
    ext = ".h"
    output = output_dir / f"{protocol.stem}{ext}"

    cmd = [
        "pixi", "run", "--", "bakelite", "gen",
        "-l", language.value,
        "-i", str(protocol),
        "-o", str(output),
    ]
    if unpacked:
        cmd.append("--unpacked")

    run_cmd(cmd)
    return output


def generate_runtime(language: Language, output_dir: Path) -> Path:
    """Generate runtime header using bakelite."""
    output = output_dir / "bakelite.h"

    run_cmd([
        "pixi", "run", "--", "bakelite", "runtime",
        "-l", language.value,
        "-o", str(output),
    ])

    return output


def get_section_sizes(obj_file: Path, size_cmd: str | Path, is_macos: bool = False) -> dict[str, int]:
    """Parse output of size command. Sums all .text.* sections for function-sections builds."""
    sizes = {"text": 0, "data": 0, "bss": 0}

    if is_macos:
        # macOS size format: __TEXT __DATA __OBJC others dec hex
        result = run_cmd([str(size_cmd), str(obj_file)])
        for line in result.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 4 and parts[0] != "__TEXT":  # skip header
                try:
                    sizes["text"] = int(parts[0])
                    sizes["data"] = int(parts[1])
                    # macOS doesn't separate bss, it's in __DATA
                except ValueError:
                    continue
    else:
        # GNU size -A format
        result = run_cmd([str(size_cmd), "-A", str(obj_file)])
        for line in result.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 2:
                try:
                    section, size = parts[0], int(parts[1])
                    # Sum all .text* sections (handles -ffunction-sections)
                    if section.startswith(".text"):
                        sizes["text"] += size
                    elif section.startswith(".data"):
                        sizes["data"] += size
                    elif section.startswith(".bss"):
                        sizes["bss"] += size
                except ValueError:
                    continue

    return sizes


def count_instructions(obj_file: Path, objdump_cmd: str | Path) -> int:
    """Count instructions in disassembly."""
    result = run_cmd([str(objdump_cmd), "-d", str(obj_file)])

    # Count lines that look like instructions
    # ARM format: "   address:    hex bytes    mnemonic"
    count = 0
    for line in result.stdout.splitlines():
        line = line.strip()
        # Instruction lines have format like: "   0:   f04f 0012       mov.w   r0, #18"
        if ":" in line and "\t" in line:
            # Skip label lines (no tab after colon)
            parts = line.split(":", 1)
            if len(parts) == 2 and "\t" in parts[1]:
                count += 1

    return count


def get_disassembly(obj_file: Path, objdump_cmd: str | Path) -> str:
    """Get full disassembly."""
    result = run_cmd([str(objdump_cmd), "-d", "-S", str(obj_file)])
    return result.stdout


def parse_stack_usage(su_file: Path) -> dict[str, int]:
    """Parse GCC stack usage file (.su)."""
    if not su_file.exists():
        return {}

    usage = {}
    for line in su_file.read_text().splitlines():
        # Format: file.c:line:col:function\tsize\ttype
        parts = line.split("\t")
        if len(parts) >= 2:
            try:
                func_info = parts[0]
                size = int(parts[1])
                # Extract function name (after last colon)
                func_name = func_info.rsplit(":", 1)[-1]
                usage[func_name] = size
            except (ValueError, IndexError):
                continue

    return usage


def find_tool(bin_dir: Path, tool_name: str, is_native: bool) -> str:
    """Find a tool either in bin_dir or system PATH."""
    if is_native:
        # For native, search PATH
        found = shutil.which(tool_name)
        if found:
            return found
        # Try alternatives for macOS
        if tool_name == "objdump":
            for alt in ["llvm-objdump", "gobjdump"]:
                found = shutil.which(alt)
                if found:
                    return found
        raise RuntimeError(f"Could not find {tool_name} in PATH")
    else:
        return str(bin_dir / tool_name)


def run_benchmark(config: BenchConfig, bin_dir: Path) -> BenchResult:
    """Run a single benchmark configuration."""
    target_cfg = TARGET_CONFIGS[config.target]
    is_native = target_cfg.toolchain == "native"

    with tempfile.TemporaryDirectory() as tmpdir_str:
        tmpdir = Path(tmpdir_str)

        protocol_path = PROTOCOLS_DIR / f"{config.protocol}.bakelite"

        # 1. Generate protocol code
        generated = generate_code(
            protocol_path, config.language, tmpdir, unpacked=target_cfg.unpacked
        )

        # 2. Generate runtime
        generate_runtime(config.language, tmpdir)

        # 3. Copy harness
        is_cpp = config.language == Language.CPPTINY
        harness_ext = ".cpp" if is_cpp else ".c"
        harness_name = f"bench_{config.protocol}_{config.language.value}{harness_ext}"
        harness_src = HARNESS_DIR / harness_name
        if not harness_src.exists():
            raise FileNotFoundError(f"Harness not found: {harness_src}")
        harness = tmpdir / harness_name
        shutil.copy(harness_src, harness)

        # Rename generated file to match include
        shutil.move(generated, tmpdir / f"{config.protocol}.h")

        # 4. Compile
        compiler_name = target_cfg.compiler
        if is_cpp:
            if is_native:
                compiler_name = "c++"
            else:
                compiler_name = compiler_name.replace("-gcc", "-g++")

        compiler = find_tool(bin_dir, compiler_name, is_native)
        obj_file = tmpdir / "bench.o"
        su_file = tmpdir / "bench.su"

        std_flag = "-std=c++14" if is_cpp else "-std=c99"
        compile_args = [
            compiler,
            "-c", str(harness),
            "-o", str(obj_file),
            f"-I{tmpdir}",
            config.opt_level.value,
            "-fstack-usage",
            "-ffunction-sections",
            "-fdata-sections",
            "-Wall",
            std_flag,
            *target_cfg.flags,
        ]

        run_cmd(compile_args, cwd=tmpdir)

        # 5. Extract metrics
        size_cmd = find_tool(bin_dir, target_cfg.size_cmd, is_native)
        objdump_cmd = find_tool(bin_dir, target_cfg.objdump_cmd, is_native)

        # macOS native uses different size format
        is_macos_native = is_native and sys.platform == "darwin"
        sizes = get_section_sizes(obj_file, size_cmd, is_macos=is_macos_native)
        inst_count = count_instructions(obj_file, objdump_cmd)
        stack = parse_stack_usage(su_file)

        # 6. Save disassembly
        disasm_dir = RESULTS_DIR / "disasm"
        disasm_dir.mkdir(parents=True, exist_ok=True)
        disasm_file = disasm_dir / f"{config.protocol}-{config.language.value}-{config.target.value}-{config.opt_level.value[1:]}.s"
        disasm = get_disassembly(obj_file, objdump_cmd)
        disasm_file.write_text(disasm)

        return BenchResult(
            config=config,
            text_size=sizes["text"],
            data_size=sizes["data"],
            bss_size=sizes["bss"],
            instruction_count=inst_count,
            stack_usage=stack,
        )


def run_all_benchmarks() -> list[BenchResult]:
    """Run benchmarks across all configurations."""
    results = []

    # Collect unique toolchains needed and try to get them
    toolchains_needed = {cfg.toolchain for cfg in TARGET_CONFIGS.values()}
    toolchain_bins: dict[str, Path | None] = {}

    for toolchain in toolchains_needed:
        bin_dir = get_or_download_toolchain(toolchain)
        toolchain_bins[toolchain] = bin_dir
        if bin_dir is None:
            print(f"Warning: {toolchain} toolchain not available, skipping those targets")

    # Find protocols that have harnesses for at least one language
    all_protocols = [p.stem for p in PROTOCOLS_DIR.glob("*.bakelite")]
    protocols = []
    for proto in all_protocols:
        ctiny_harness = HARNESS_DIR / f"bench_{proto}_ctiny.c"
        cpptiny_harness = HARNESS_DIR / f"bench_{proto}_cpptiny.cpp"
        if ctiny_harness.exists() or cpptiny_harness.exists():
            protocols.append(proto)
        else:
            print(f"Skipping {proto} (no harness)")

    if not protocols:
        print("No protocols with harnesses found")
        return results

    # Run benchmarks
    for protocol in protocols:
        for lang in Language:
            # Check if harness exists for this language
            harness_ext = ".cpp" if lang == Language.CPPTINY else ".c"
            harness = HARNESS_DIR / f"bench_{protocol}_{lang.value}{harness_ext}"
            if not harness.exists():
                continue

            for target in Target:
                target_cfg = TARGET_CONFIGS[target]
                bin_dir = toolchain_bins[target_cfg.toolchain]

                # Skip if toolchain not available
                if bin_dir is None:
                    continue

                for opt in [OptLevel.Os, OptLevel.O2]:
                    config = BenchConfig(
                        protocol=protocol,
                        language=lang,
                        target=target,
                        opt_level=opt,
                    )
                    print(f"Benchmarking: {protocol}/{lang.value}/{target.value}/{opt.value}")
                    try:
                        result = run_benchmark(config, bin_dir)
                        results.append(result)
                        print(f"  .text={result.text_size} bytes, {result.instruction_count} instructions")
                    except Exception as e:
                        print(f"  FAILED: {e}")

    return results


def save_results(results: list[BenchResult]) -> Path:
    """Save results to JSON."""
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    data = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "results": [
            {
                "protocol": r.config.protocol,
                "language": r.config.language.value,
                "target": r.config.target.value,
                "opt_level": r.config.opt_level.value,
                "text_size": r.text_size,
                "data_size": r.data_size,
                "bss_size": r.bss_size,
                "instruction_count": r.instruction_count,
                "stack_usage": r.stack_usage,
            }
            for r in results
        ],
    }

    output = RESULTS_DIR / "latest.json"
    output.write_text(json.dumps(data, indent=2))
    print(f"\nResults saved to: {output}")

    return output


def print_summary(results: list[BenchResult]) -> None:
    """Print a summary table."""
    if not results:
        return

    print("\n" + "=" * 80)
    print("BENCHMARK SUMMARY")
    print("=" * 80)
    print(
        f"{'Protocol':<10} {'Target':<15} {'Opt':<4} "
        f"{'Text':>6} {'RAM':>6} {'Instr':>6} {'Stack':>6}"
    )
    print("-" * 80)

    for r in results:
        ram = r.data_size + r.bss_size
        # Max stack across all functions
        max_stack = max(r.stack_usage.values()) if r.stack_usage else 0
        print(
            f"{r.config.protocol:<10} "
            f"{r.config.target.value:<15} "
            f"{r.config.opt_level.value:<4} "
            f"{r.text_size:>6} "
            f"{ram:>6} "
            f"{r.instruction_count:>6} "
            f"{max_stack:>6}"
        )


def main() -> None:
    import argparse  # noqa: PLC0415

    parser = argparse.ArgumentParser(description="Run bakelite benchmarks")
    parser.add_argument("--protocol", help="Specific protocol to benchmark")

    _args = parser.parse_args()  # TODO: use --protocol filter

    results = run_all_benchmarks()

    if results:
        save_results(results)
        print_summary(results)
    else:
        print("No results generated")
        sys.exit(1)


if __name__ == "__main__":
    main()
