#!/usr/bin/env python3
"""Download and manage cross-compiler toolchains for benchmarking."""

import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
from dataclasses import dataclass
from pathlib import Path

BENCH_DIR = Path(__file__).parent.parent
TOOLCHAINS_DIR = BENCH_DIR / "toolchains"


def get_platform() -> str:
    """Returns platform key like 'darwin-arm64' or 'linux-x86_64'."""
    os_name = platform.system().lower()
    arch = platform.machine()
    # Normalize architecture names
    if arch == "aarch64":
        arch = "arm64"
    return f"{os_name}-{arch}"


@dataclass
class ToolchainInfo:
    name: str
    urls: dict[str, str]  # platform -> url
    bin_subdir: str = "bin"  # path to bin directory inside extracted archive


# ARM toolchain from ARM Developer
# https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
TOOLCHAINS: dict[str, ToolchainInfo] = {
    "arm-none-eabi": ToolchainInfo(
        name="arm-none-eabi",
        urls={
            "darwin-arm64": "https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi.tar.xz",
            "darwin-x86_64": "https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-darwin-x86_64-arm-none-eabi.tar.xz",
            "linux-x86_64": "https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz",
            "linux-arm64": "https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-aarch64-arm-none-eabi.tar.xz",
        },
    ),
    # AVR toolchain from ZakKemble/avr-gcc-build
    # https://github.com/ZakKemble/avr-gcc-build/releases
    # Note: No macOS builds available - use `brew install avr-gcc` on macOS
    "avr": ToolchainInfo(
        name="avr",
        urls={
            "linux-x86_64": "https://github.com/ZakKemble/avr-gcc-build/releases/download/v15.2.0-1/avr-gcc-15.2.0-x64-linux.tar.bz2",
        },
    ),
    # RISC-V toolchain from xPack
    # https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases
    "riscv-none-elf": ToolchainInfo(
        name="riscv-none-elf",
        urls={
            "darwin-arm64": "https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v15.2.0-1/xpack-riscv-none-elf-gcc-15.2.0-1-darwin-arm64.tar.gz",
            "darwin-x86_64": "https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v15.2.0-1/xpack-riscv-none-elf-gcc-15.2.0-1-darwin-x64.tar.gz",
            "linux-x86_64": "https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v15.2.0-1/xpack-riscv-none-elf-gcc-15.2.0-1-linux-x64.tar.gz",
            "linux-arm64": "https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v15.2.0-1/xpack-riscv-none-elf-gcc-15.2.0-1-linux-arm64.tar.gz",
        },
    ),
}


def download_with_progress(url: str, dest: Path) -> None:
    """Download a file with progress reporting."""
    print(f"Downloading: {url}")

    def report(block_num: int, block_size: int, total_size: int) -> None:
        downloaded = block_num * block_size
        if total_size > 0:
            percent = min(100, downloaded * 100 // total_size)
            mb_down = downloaded / (1024 * 1024)
            mb_total = total_size / (1024 * 1024)
            print(f"\r  {percent}% ({mb_down:.1f}/{mb_total:.1f} MB)", end="", flush=True)

    urllib.request.urlretrieve(url, dest, reporthook=report)
    print()  # newline after progress


def find_bin_dir(extract_dir: Path, toolchain_name: str) -> Path:
    """Find the bin directory inside extracted archive (may be nested)."""
    # Determine which gcc to look for based on toolchain
    gcc_patterns = {
        "arm-none-eabi": "arm-none-eabi-gcc",
        "avr": "avr-gcc",
        "riscv-none-elf": "riscv-none-elf-gcc",
    }
    gcc_name = gcc_patterns.get(toolchain_name, f"{toolchain_name}-gcc")

    for gcc in extract_dir.rglob(gcc_name):
        if gcc.is_file():
            return gcc.parent
    raise RuntimeError(f"Could not find {gcc_name} in {extract_dir}")


def download_toolchain(name: str, force: bool = False) -> Path:
    """Download and extract toolchain, return bin directory path."""
    if name not in TOOLCHAINS:
        raise ValueError(f"Unknown toolchain: {name}. Available: {list(TOOLCHAINS.keys())}")

    info = TOOLCHAINS[name]
    platform_key = get_platform()

    if platform_key not in info.urls:
        raise RuntimeError(f"No {name} toolchain available for {platform_key}")

    dest_dir = TOOLCHAINS_DIR / name

    # Check if already downloaded
    if dest_dir.exists() and not force:
        bin_dir = find_bin_dir(dest_dir, name)
        print(f"Toolchain {name} already exists at {bin_dir}")
        return bin_dir

    # Remove existing if forcing
    if dest_dir.exists():
        shutil.rmtree(dest_dir)

    url = info.urls[platform_key]
    TOOLCHAINS_DIR.mkdir(parents=True, exist_ok=True)

    # Download to temp file
    suffix = ".tar.gz" if url.endswith(".tar.gz") else ".tar.xz"
    with tempfile.NamedTemporaryFile(suffix=suffix, delete=False) as tmp:
        tmp_path = Path(tmp.name)

    try:
        download_with_progress(url, tmp_path)

        print(f"Extracting to {dest_dir}...")
        dest_dir.mkdir(parents=True, exist_ok=True)

        # Extract - handle both .tar.xz and .tar.gz
        with tarfile.open(tmp_path) as tar:
            tar.extractall(dest_dir, filter="data")

    finally:
        tmp_path.unlink(missing_ok=True)

    bin_dir = find_bin_dir(dest_dir, name)
    print(f"Installed {name} toolchain: {bin_dir}")

    # Verify it works
    gcc_patterns = {
        "arm-none-eabi": "arm-none-eabi-gcc",
        "avr": "avr-gcc",
        "riscv-none-elf": "riscv-none-elf-gcc",
    }
    gcc_name = gcc_patterns.get(name, f"{name}-gcc")
    gcc_path = bin_dir / gcc_name
    result = subprocess.run([str(gcc_path), "--version"], check=False, capture_output=True, text=True)
    if result.returncode == 0:
        version_line = result.stdout.splitlines()[0]
        print(f"  Version: {version_line}")
    else:
        print(f"  Warning: could not verify toolchain: {result.stderr}")

    return bin_dir


def get_toolchain_bin(name: str) -> Path | None:
    """Get path to toolchain bin directory if installed, None otherwise."""
    dest_dir = TOOLCHAINS_DIR / name
    if not dest_dir.exists():
        return None
    try:
        return find_bin_dir(dest_dir, name)
    except RuntimeError:
        return None


def find_system_toolchain(name: str) -> Path | None:
    """Check if toolchain is available in system PATH."""
    gcc_patterns = {
        "arm-none-eabi": "arm-none-eabi-gcc",
        "avr": "avr-gcc",
        "riscv-none-elf": "riscv-none-elf-gcc",
    }
    gcc_name = gcc_patterns.get(name, f"{name}-gcc")

    # Check if available in PATH
    gcc_path = shutil.which(gcc_name)
    if gcc_path:
        return Path(gcc_path).parent
    return None


def get_or_download_toolchain(name: str) -> Path | None:
    """Get toolchain bin dir - from download, system, or None if unavailable."""
    # Native toolchain uses system PATH directly
    if name == "native":
        return Path("/usr/bin")  # Placeholder - commands found via PATH

    # First check if already downloaded
    bin_dir = get_toolchain_bin(name)
    if bin_dir:
        return bin_dir

    # Check system PATH
    bin_dir = find_system_toolchain(name)
    if bin_dir:
        print(f"Using system {name} toolchain from {bin_dir}")
        return bin_dir

    # Try to download
    if name in TOOLCHAINS:
        platform_key = get_platform()
        if platform_key in TOOLCHAINS[name].urls:
            return download_toolchain(name)

    return None


def main() -> None:
    import argparse  # noqa: PLC0415

    parser = argparse.ArgumentParser(description="Download cross-compiler toolchains")
    parser.add_argument(
        "toolchain",
        nargs="?",
        default="arm-none-eabi",
        choices=list(TOOLCHAINS.keys()),
        help="Toolchain to download (default: arm-none-eabi)",
    )
    parser.add_argument("--force", action="store_true", help="Re-download even if exists")
    parser.add_argument("--list", action="store_true", help="List available toolchains")

    args = parser.parse_args()

    if args.list:
        print("Available toolchains:")
        for name, info in TOOLCHAINS.items():
            platforms = ", ".join(info.urls.keys())
            print(f"  {name}: {platforms}")
        return

    platform_key = get_platform()
    print(f"Platform: {platform_key}")

    try:
        bin_dir = download_toolchain(args.toolchain, force=args.force)
        print(f"\nToolchain ready: {bin_dir}")
    except RuntimeError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
