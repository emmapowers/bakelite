from typing_extensions import Required
import click
from click.decorators import command

import bakelite.generator.python as python
import bakelite.generator.cpptiny as cpptiny
from bakelite.generator import parse

@click.command()
@click.option('--language', '-l', required=True)
@click.option('--input', '-i', required=True)
@click.option('--output', '-o', required=True)
def main(language: str, input: str, output: str):
  render_func = None

  if language == "python":
    render_func = python.render
  elif language == "cpptiny":
    render_func = cpptiny.render
  else:
    print(f"Unkown language: {language}")
    return 1

  with open(input, 'r', encoding='utf-8') as f:
    proto = f.read()

  proto_def = parse(proto)
  generated_file = render_func(*proto_def)

  with open(output, 'w', encoding='utf-8') as f:
    f.write(generated_file)
    


if __name__ == "__main__":
  main()