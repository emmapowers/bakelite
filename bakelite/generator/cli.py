"""Command-line interface for bakelite code generation."""

import sys
from pathlib import Path

import click

from bakelite.generator import cpptiny, parse, python


@click.group()
def cli() -> None:
    """Bakelite protocol code generator."""


@cli.command()
@click.option("--language", "-l", required=True, help="Target language (python, cpptiny)")
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
    else:
        print(f"Unknown language: {language}")
        sys.exit(1)

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(generated_file)


@cli.command()
@click.option("--language", "-l", required=True, help="Target language (python, cpptiny)")
@click.option("--output", "-o", "output_path", default=".", help="Output directory")
@click.option("--name", default="bakelite_runtime", help="Runtime folder name (python only)")
def runtime(language: str, output_path: str, name: str) -> None:
    """Generate runtime support code."""
    if language == "cpptiny":
        generated_file = cpptiny.runtime()
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


def main() -> None:
    """Main entry point."""
    cli()


if __name__ == "__main__":
    main()
