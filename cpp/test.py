from bakelite.proto import parse, List
from bakelite.proto.generator.cpptiny import render

with open('./struct.ex') as f:
  text = f.read()

proto = parse(text)

code = render(*proto)
print(code)