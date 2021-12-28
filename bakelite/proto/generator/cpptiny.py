from jinja2 import Environment, PackageLoader
from dataclasses import asdict
from copy import copy

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

template = env.get_template('cpptiny.h.j2')

prim_types = {
    "bool": "bool",
    "int8": "int8_t",
    "int16": "int16_t",
    "int32": "int32_t",
    "int64": "int64_t",
    "uint8": "uint8_t",
    "uint16": "uint16_t",
    "uint32": "uint32_t",
    "uint64": "uint64_t",
    "float32": "float",
    "float64": "double",
    "bytes": "uint8_t",
    "string": "char",
  }

def _map_type(t: ProtoType):
  if t.name in prim_types:
    type_name = prim_types[t.name]
  else:
    type_name = t.name
  
  return type_name

def _map_type_member(member: ProtoStructMember, use_pointer=False) -> str:
  type_name = _map_type(member.type)
  
  if member.type.name == "bytes" and member.type.size == 0 and member.arraySize == 0:
    return f"Bakelite::SizedArray<Bakelite::SizedArray<{type_name}> >"
  elif member.type.name == "bytes" and member.type.size == 0:
    return f"Bakelite::SizedArray<{type_name}>"
  if member.type.name == "string" and (member.type.size == 0 or use_pointer == True) and member.arraySize == 0:
    return f"Bakelite::SizedArray<{type_name}*>"
  if member.type.name == "string" and (member.type.size == 0 or use_pointer == True):
    return f"{type_name}*"
  elif member.arraySize == 0:
    return f"Bakelite::SizedArray<{type_name}>"
  else:
    return type_name


def _size_postfix(member: ProtoStructMember) -> str:
  if member.type.name == "bytes" or member.type.name == "string":
    if member.type.size == 0:
      return ''
    else:
      return f"[{member.type.size}]"
  else:
    return ''

def _array_postfix(member: ProtoStructMember) -> str:
  if member.arraySize is None or member.arraySize == 0:
    return ''
  else:
    return f"[{member.arraySize}]"


def render(enums: List[ProtoEnum],
          structs: List[ProtoStruct],
          proto: Protocol,
          comments: List[str]) -> str:

  enums_types = { enum.name: enum for enum in enums }
  structs_types = { struct.name: struct for struct in structs }

  def _write_type(member: ProtoStructMember):
    if member.arraySize is not None:
      size_arg = f', {member.arraySize}' if member.arraySize > 0 else ''
      tmp_member = copy(member)
      tmp_member.arraySize = None
      tmp_member.name = "val"
      tmp_member_type = _map_type_member(tmp_member, use_pointer=True)
      return (
f"""writeArray(stream, {member.name}{size_arg}, [](T &stream, {tmp_member_type} const &val) {{
      return {_write_type(tmp_member)}
    }});""")
    elif member.type.name in enums_types:
      underlying_type = _map_type(enums_types[member.type.name])
      return f"write(stream, ({underlying_type}){member.name});"
    elif member.type.name in structs_types:
      return f"{member.name}.pack(stream);"
    elif member.type.name in prim_types and member.type.name != "bytes" and member.type.name != "string":
      return f"write(stream, {member.name});"
    elif member.type.name == "bytes":
      if member.type.size != 0:
        return f"writeBytes(stream, {member.name}, {member.type.size});"
      else:
        return f"writeBytes(stream, {member.name});"
    elif member.type.name == "string":
      if member.type.size != 0:
        return f"writeString(stream, {member.name}, {member.type.size});"
      else:
        return f"writeString(stream, {member.name});"
    else:
      raise RuntimeError(f"Unkown type {member.type.name}")

  def _read_type(member: ProtoStructMember):
    if member.arraySize is not None:
      size_arg = f', {member.arraySize}' if member.arraySize > 0 else ''
      tmp_member = copy(member)
      tmp_member.arraySize = None
      tmp_member.name = "val"
      tmp_member_type = _map_type_member(tmp_member, use_pointer=True)
      return (
f"""readArray(stream, {member.name}{size_arg}, [](T &stream, {tmp_member_type} &val) {{
      return {_read_type(tmp_member)}
    }});""")
    elif member.type.name in enums_types:
      underlying_type = _map_type(enums_types[member.type.name])
      return f"read(stream, ({underlying_type}){member.name});"
    elif member.type.name in structs_types:
      return f"{member.name}.unpack(stream);"
    elif member.type.name in prim_types and member.type.name != "bytes" and member.type.name != "string":
      return f"read(stream, {member.name});"
    elif member.type.name == "bytes":
      if member.type.size != 0:
        return f"readBytes(stream, {member.name}, {member.type.size});"
      else:
        return f"readBytes(stream, {member.name});"
    elif member.type.name == "string":
      if member.type.size != 0:
        return f"readString(stream, {member.name}, {member.type.size});"
      else:
        return f"readString(stream, {member.name});"
    else:
      raise RuntimeError(f"Unkown type {member.type.name}")


  return template.render(
    enums=enums,
    structs=structs,
    proto=proto,
    comments=comments,
    map_type=_map_type,
    map_type_member=_map_type_member,
    array_postfix=_array_postfix,
    size_postfix=_size_postfix,
    write_type=_write_type,
    read_type=_read_type,
    to_camel_case=to_camel_case)
