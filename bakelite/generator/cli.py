# pylint: disable=redefined-builtin

import sys

import click

from bakelite.generator import cpptiny, parse, python


@click.group()
def cli() -> None:
    pass


@cli.command()
@click.option("--language", "-l", required=True)
@click.option("--input", "-i", required=True)
@click.option("--output", "-o", required=True)
def gen(language: str, input: str, output: str) -> None:
    with open(input, "r", encoding="utf-8") as f:
        proto = f.read()

    proto_def = parse(proto)

    if language == "python":
        generated_file = python.render(*proto_def)
    elif language == "cpptiny":
        generated_file = cpptiny.render(*proto_def)
    else:
        print(f"Unknown language: {language}")
        sys.exit(1)

    with open(output, "w", encoding="utf-8") as f:
        f.write(generated_file)


@cli.command()
@click.option("--language", "-l", required=True)
@click.option("--output", "-o", required=True)
def runtime(language: str, output: str) -> None:
    runtime_func = None

    if language == "cpptiny":
        runtime_func = cpptiny.runtime
    else:
        print(f"Unkown language: {language}")
        sys.exit(1)

    generated_file = runtime_func()

    with open(output, "w", encoding="utf-8") as f:
        f.write(generated_file)


def main() -> None:
    cli()


if __name__ == "__main__":
    main()
