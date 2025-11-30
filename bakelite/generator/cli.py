"""Command-line interface for bakelite code generation."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import TYPE_CHECKING

import click
from rich.console import Console
from rich.table import Table

from bakelite.generator import cpptiny, ctiny, parse, python
from bakelite.generator.sizes import ProtocolSizeInfo, calculate_sizes

if TYPE_CHECKING:
    from bakelite.generator.types import Protocol


@click.group()
def cli() -> None:
    """Bakelite protocol code generator."""


@cli.command()
@click.option("--language", "-l", required=True, help="Target language (python, cpptiny, ctiny)")
@click.option("--input", "-i", "input_file", required=True, help="Input protocol file")
@click.option("--output", "-o", "output_file", required=True, help="Output file")
@click.option(
    "--runtime-import",
    "runtime_import",
    is_flag=False,
    flag_value="bakelite.proto",
    default=None,
    help="Import path for runtime. No value=bakelite.proto, omit=runtime",
)
@click.option(
    "--unpacked",
    is_flag=True,
    default=False,
    help="Generate aligned structs with memmove shuffle (for Cortex-M0, RISC-V, ESP32, PIC32)",
)
def gen(
    language: str, input_file: str, output_file: str, runtime_import: str | None, unpacked: bool
) -> None:
    """Generate protocol code from a definition file."""
    with open(input_file, encoding="utf-8") as f:
        proto = f.read()

    proto_def = parse(proto)

    if language == "python":
        # Default to "bakelite_runtime" (relative import) if not specified
        import_path = runtime_import if runtime_import is not None else "bakelite_runtime"
        generated_file = python.render(*proto_def, runtime_import=import_path)
    elif language == "cpptiny":
        generated_file = cpptiny.render(*proto_def, unpacked=unpacked)
    elif language == "ctiny":
        generated_file = ctiny.render(*proto_def, unpacked=unpacked)
    else:
        print(f"Unknown language: {language}")
        sys.exit(1)

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(generated_file)


@cli.command()
@click.option("--language", "-l", required=True, help="Target language (python, cpptiny, ctiny)")
@click.option("--output", "-o", "output_path", default=".", help="Output directory")
@click.option("--name", default="bakelite_runtime", help="Runtime folder name (python only)")
def runtime(language: str, output_path: str, name: str) -> None:
    """Generate runtime support code."""
    if language == "cpptiny":
        generated_file = cpptiny.runtime()
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(generated_file)
    elif language == "ctiny":
        generated_file = ctiny.runtime()
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(generated_file)
    elif language == "python":
        runtime_dir = Path(output_path) / name
        runtime_dir.mkdir(parents=True, exist_ok=True)
        for filename, content in python.runtime().items():
            (runtime_dir / filename).write_text(content)
        print(f"Generated Python runtime in {runtime_dir}")
    else:
        print(f"Unknown language: {language}")
        sys.exit(1)


@cli.command()
@click.option("--input", "-i", "input_file", required=True, help="Input protocol file")
@click.option("--json", "output_json", is_flag=True, help="Output as JSON")
def info(input_file: str, output_json: bool) -> None:
    """Display protocol information and size calculations."""
    with open(input_file, encoding="utf-8") as f:
        proto = f.read()

    enums, structs, protocol, _comments = parse(proto)
    size_info = calculate_sizes(enums, structs, protocol)

    if output_json:
        _output_json(size_info, protocol, enums)
    else:
        _output_plain(size_info, protocol, enums)


def _format_size(size: int | None) -> str:
    """Format a size value, handling None for unbounded."""
    return "unbounded" if size is None else str(size)


def _output_json(
    size_info: ProtocolSizeInfo,
    protocol: Protocol | None,
    enums: list,
) -> None:
    """Output protocol info as JSON."""
    data: dict = {
        "protocol": {},
        "structs": {},
        "messages": {},
        "ram": {},
    }

    if protocol:
        options = {opt.name: opt.value for opt in protocol.options}
        data["protocol"] = {
            "framing": options.get("framing", "none"),
            "crc": options.get("crc", "none"),
            "maxLength": options.get("maxLength"),
            "crc_size": size_info.crc_size,
        }

    for name, struct_info in size_info.structs.items():
        data["structs"][name] = {
            "min_size": struct_info.size.min_size,
            "max_size": struct_info.size.max_size,
            "kind": struct_info.size.kind.value,
        }

    for name, struct_info in size_info.message_structs.items():
        data["messages"][name] = {
            "min_size": struct_info.size.min_size,
            "max_size": struct_info.size.max_size,
        }

    data["ram"] = {
        "min_message_size": size_info.min_message_size,
        "max_message_size": size_info.max_message_size,
        "framing_overhead": size_info.framing_overhead,
        "required_buffer_size": size_info.required_buffer_size,
        "estimated_ram_ctiny": size_info.estimated_ram_ctiny,
        "estimated_ram_cpptiny": size_info.estimated_ram_cpptiny,
    }

    print(json.dumps(data, indent=2))


def _output_plain(
    size_info: ProtocolSizeInfo,
    protocol: Protocol | None,
    enums: list,
) -> None:
    """Output protocol info using rich text formatting."""
    console = Console()

    # Protocol settings
    if protocol:
        options = {opt.name: opt.value for opt in protocol.options}
        console.print("[bold cyan]Protocol[/bold cyan]")

        proto_table = Table(show_header=False, box=None, padding=(0, 2, 0, 2))
        proto_table.add_column("Label", style="dim")
        proto_table.add_column("Value", style="white")

        proto_table.add_row("Framing", options.get("framing", "none").upper())

        crc = options.get("crc", "none")
        crc_str = (
            f"{crc.upper()} ({size_info.crc_size} byte{'s' if size_info.crc_size != 1 else ''})"
        )
        proto_table.add_row("CRC", crc_str)

        max_len = options.get("maxLength")
        max_len_str = str(max_len) if max_len else f"auto ({size_info.max_message_size})"
        proto_table.add_row("Max Length", max_len_str)

        console.print(proto_table)
        console.print()

    # Build message ID lookup
    message_ids: dict[str, int] = {}
    if protocol:
        message_ids = {msg.name: msg.number for msg in protocol.message_ids}

    # Struct sizes
    console.print("[bold cyan]Structs[/bold cyan]")
    struct_table = Table(show_header=True, box=None, padding=(0, 2, 0, 0))
    struct_table.add_column("Name", style="white")
    struct_table.add_column("Size", style="yellow", justify="right")
    struct_table.add_column("Kind", style="dim")
    struct_table.add_column("Msg ID", style="green", justify="right")

    for name, struct_info in size_info.structs.items():
        min_size = struct_info.size.min_size
        max_size = struct_info.size.max_size
        kind = struct_info.size.kind.value

        if min_size == max_size:
            size_str = f"{min_size} bytes"
        else:
            size_str = f"{min_size}-{_format_size(max_size)} bytes"

        msg_id = str(message_ids[name]) if name in message_ids else ""
        struct_table.add_row(name, size_str, kind, msg_id)

    console.print(struct_table)
    console.print()

    # Embedded RAM requirements
    console.print("[bold cyan]Embedded RAM (ctiny/cpptiny)[/bold cyan]")
    ram_table = Table(show_header=False, box=None, padding=(0, 2, 0, 2))
    ram_table.add_column("Label", style="dim")
    ram_table.add_column("Value", style="white")

    ram_table.add_row("Buffer", f"{_format_size(size_info.required_buffer_size)} bytes")
    if size_info.estimated_ram_ctiny is not None:
        ram_table.add_row("Total RAM", f"~{size_info.estimated_ram_ctiny} bytes")

    console.print(ram_table)


def main() -> None:
    """Main entry point."""
    cli()


if __name__ == "__main__":
    main()
