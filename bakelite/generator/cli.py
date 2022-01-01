# pylint: disable=redefined-builtin

import sys

import click

import bakelite.generator.cpptiny as cpptiny
import bakelite.generator.python as python
from bakelite.generator import parse


@click.group()
def cli():
  pass


@cli.command()
@click.option('--language', '-l', required=True)
@click.option('--input', '-i', required=True)
@click.option('--output', '-o', required=True)
def gen(language: str, input: str, output: str):
  render_func = None

  if language == "python":
    render_func = python.render
  elif language == "cpptiny":
    render_func = cpptiny.render
  else:
    print(f"Unkown language: {language}")
    sys.exit(1)

  with open(input, 'r', encoding='utf-8') as f:
    proto = f.read()

  proto_def = parse(proto)
  generated_file = render_func(*proto_def)

  with open(output, 'w', encoding='utf-8') as f:
    f.write(generated_file)


@cli.command()
@click.option('--language', '-l', required=True)
@click.option('--output', '-o', required=True)
def runtime(language: str, output: str):
  runtime_func = None

  if language == "cpptiny":
    runtime_func = cpptiny.runtime
  else:
    print(f"Unkown language: {language}")
    sys.exit(1)

  generated_file = runtime_func()

  with open(output, 'w', encoding='utf-8') as f:
    f.write(generated_file)


def main():
  cli()


if __name__ == "__main__":
  main()
