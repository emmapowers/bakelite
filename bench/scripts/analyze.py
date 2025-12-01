#!/usr/bin/env python3
"""Generate markdown summary from benchmark results."""

import json
from datetime import UTC, datetime
from pathlib import Path

BENCH_DIR = Path(__file__).parent.parent
RESULTS_DIR = BENCH_DIR / "results"

# Target descriptions: (display_name, packed/unpacked mode)
TARGET_INFO = {
    "arm-cortex-m0": ("Cortex-M0", "unpacked"),
    "arm-cortex-m3": ("Cortex-M3", "packed"),
    "avr-atmega328p": ("ATmega328P", "unpacked"),
    "riscv32-imc": ("RV32IMC", "unpacked"),
    "native-x64": ("x86-64", "packed"),
}


def generate_summary() -> str:
    results_file = RESULTS_DIR / "latest.json"
    if not results_file.exists():
        return "No results found. Run benchmarks first."

    with open(results_file) as f:
        data = json.load(f)

    lines = [
        "# Bakelite Benchmark Results",
        "",
        f"Generated: {datetime.now(UTC).strftime('%Y-%m-%d')}",
        "",
    ]

    # Group by protocol
    by_protocol: dict[str, list] = {}
    for r in data["results"]:
        proto = r["protocol"]
        if proto not in by_protocol:
            by_protocol[proto] = []
        by_protocol[proto].append(r)

    for proto, results in by_protocol.items():
        lines.append(f"## Protocol: {proto}")
        lines.append("")
        lines.append("| Lang | Target | Mode | Opt | Text | RAM | Instructions | Max Stack |")
        lines.append("|------|--------|------|-----|-----:|----:|-------------:|----------:|")

        for r in results:
            lang = r["language"]
            target = r["target"]
            target_name, mode = TARGET_INFO.get(target, (target, "?"))
            opt = r["opt_level"]
            text = r["text_size"]
            ram = r["data_size"] + r["bss_size"]
            instr = r["instruction_count"]
            stack = max(r["stack_usage"].values()) if r["stack_usage"] else 0

            lines.append(
                f"| {lang} | {target_name} | {mode} | {opt} | {text} B | {ram} B | {instr} | {stack} B |"
            )

        lines.append("")

    return "\n".join(lines)


def main() -> None:
    summary = generate_summary()
    print(summary)

    output = RESULTS_DIR / "summary.md"
    output.write_text(summary)
    print(f"\nSaved to: {output}")


if __name__ == "__main__":
    main()
