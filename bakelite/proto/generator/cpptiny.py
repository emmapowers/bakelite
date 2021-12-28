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

def _map_type_member(member: ProtoStructMember) -> str:
  type_name = _map_type(member.type)
  
  if member.arraySize is None or member.arraySize > 0:
    return type_name
  elif member.arraySize == 0:
    return f"{type_name}*"


def _size_postfix(member: ProtoStructMember) -> str:
  if member.type.name == "bytes" or member.type.name == "string":
    if member.type.size == 0:
      return []
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
    errorChecking = """
    if(rcode != 0)
      return rcode;
"""

    if member.arraySize is not None:
      pass
    elif member.type.name in enums_types:
      underlying_type = enums_types[member.type.name].type
      return f"rcode = write(stream, ({underlying_type.name}){member.name});{errorChecking}"
    elif member.type.name in structs_types:
      return f"rcode = {member.name}.pack(stream);{errorChecking}"
    elif member.type.name in prim_types and member.type.name != "bytes" and member.type.name != "string":
      return f"rcode = write(stream, {member.name});{errorChecking}"
    elif member.type.name == "bytes":
      if member.type.size != 0:
        return f"rcode = writeBytesFixed(stream, {member.name}, {member.type.size});{errorChecking}"
      else:
        return f"rcode = writeBytes(stream, {member.name}.data, {member.name}.size);{errorChecking}"
    elif member.type.name == "string":
      if member.type.size != 0:
        return f"rcode = writeStringFixed(stream, {member.name}, {member.type.size});{errorChecking}"
      else:
        return f"rcode = writeString(stream, {member.name}.data);{errorChecking}"
    else:
      raise RuntimeError(f"Unkown type {member.type.name}")

  def _read_type(member: ProtoStructMember):
    errorChecking = """
    if(rcode != 0)
      return rcode;
"""

    if member.arraySize is not None:
      pass
    elif member.type.name in enums_types:
      underlying_type = enums_types[member.type.name].type
      return f"rcode = read(stream, ({underlying_type.name}){member.name});{errorChecking}"
    elif member.type.name in structs_types:
      return f"rcode = {member.name}.unpack(stream);{errorChecking}"
    elif member.type.name in prim_types and member.type.name != "bytes" and member.type.name != "string":
      return f"rcode = read(stream, {member.name});{errorChecking}"
    elif member.type.name == "bytes":
      if member.type.size != 0:
        return f"rcode = readBytesFixed(stream, {member.name}, {member.type.size});{errorChecking}"
      else:
        return f"rcode = readBytes(stream, {member.name}.data, {member.name}.size);{errorChecking}"
    elif member.type.name == "string":
      if member.type.size != 0:
        return f"rcode = readStringFixed(stream, {member.name}, {member.type.size});{errorChecking}"
      else:
        return f"rcode = readString(stream, {member.name}.data);{errorChecking}"
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
