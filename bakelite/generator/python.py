from jinja2 import Environment, PackageLoader

from .types import *
from .util import to_camel_case


env = Environment(
    loader=PackageLoader('bakelite.generator', 'templates'),
    trim_blocks=True,
    lstrip_blocks=True,
    keep_trailing_newline=True,
    line_comment_prefix='%%',
    line_statement_prefix='%',
)

template = env.get_template('python.py.j2')


def _map_type(member: ProtoStructMember) -> str:
  prim_types = {
      "bool": "bool",
      "int8": "int",
      "int16": "int",
      "int32": "int",
      "int64": "int",
      "uint8": "int",
      "uint16": "int",
      "uint32": "int",
      "uint64": "int",
      "float32": "float",
      "float64": "float",
      "bytes": "bytes",
      "string": "str",
  }

  if member.type.name in prim_types:
    type_name = prim_types[member.type.name]
  else:
    type_name = member.type.name

  if member.arraySize is not None:
    return f"List[{type_name}]"
  else:
    return type_name


def _to_desc(dclass):
  return dclass.to_json()


def render(
    enums: List[ProtoEnum],
    structs: List[ProtoStruct],
    proto: Protocol,
    comments: List[str],
) -> str:

  return template.render(
      enums=enums,
      structs=structs,
      proto=proto,
      comments=comments,
      map_type=_map_type,
      to_desc=_to_desc,
      to_camel_case=to_camel_case,
  )
