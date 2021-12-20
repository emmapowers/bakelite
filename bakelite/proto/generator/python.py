from jinja2 import Environment, PackageLoader
from dataclasses import asdict

from ..types import *
from typing import List
from ..util import to_camel_case

env = Environment(
    loader=PackageLoader('bakelite.proto.generator', 'templates'),
    trim_blocks=True,
    lstrip_blocks=True,
    keep_trailing_newline=True,
    line_comment_prefix='%%',
    line_statement_prefix='%'
)

template = env.get_template('python.py.j2')

def _map_type(member: ProtoStructMember) -> str:
  prim_types = {
    "flag": "bool",
    "int": "int",
    "uint": "int",
    "float": "float",
    "bits": "bytes",
    "bytes": "bytes",
    "string": "str",
    "unused": "None",
  }

  if member.type.name in prim_types:
    return prim_types[member.type.name]
  
  if member.arraySize is not None:
    return f"List[{member.type.name}]"
  else:
    return member.type.name

def _to_desc(dclass):
  return dclass.to_json()

def render(enums: List[ProtoEnum],
           structs: List[ProtoStruct],
           proto: Protocol,
           comments: List[str]) -> str:

  return template.render(
    enums=enums,
    structs=structs,
    proto=proto,
    comments=comments,
    map_type=_map_type,
    to_desc=_to_desc,
    to_camel_case=to_camel_case)
