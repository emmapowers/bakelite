import struct as pystruct
from typing import Any
from dataclasses import is_dataclass, fields, field
from enum import Enum
from functools import partial
from io import BufferedIOBase

from .types import ProtoStruct, ProtoEnum, ProtoStructMember, ProtoType, is_primitive
from .runtime import Registry


class SerializationError(RuntimeError):
  pass

def _pack_type(stream: BufferedIOBase, value: Any, t: ProtoType, registry: Registry) -> None:
  if is_primitive(t):
    _pack_primitive_type(stream, value, t)
  else:
    if registry.is_enum(t.name):
      # Serialize the enums underlying type
      _pack_type(stream, value.value, registry.get(t.name)._desc.type, registry)
    elif registry.is_struct(t.name):
      value.pack(stream)
    else:
      raise SerializationError(f'{t.name} is not a primitive type, struct, or enum')

def _pack_primitive_type(stream: BufferedIOBase, value: Any, t: ProtoType) -> None:
  data: bytes = b''
  format_str: str = '='

  if(t.name == "bool"):
    format_str += "?"
  elif(t.name == "int8"):
    format_str += "b"
  elif(t.name == "uint8"):
    format_str += "B"
  elif(t.name == "int16"):
    format_str += "h"
  elif(t.name == "uint16"):
    format_str += "H"
  elif(t.name == "int32"):
    format_str += "i"
  elif(t.name == "uint32"):
    format_str += "I"
  elif(t.name == "int64"):
    format_str += "q"
  elif(t.name == "uint64"):
    format_str += "Q"
  elif(t.name == "float32"):
    format_str += "f"
  elif(t.name == "float64"):
    format_str += "d"
  elif(t.name == "bytes"):
    print(value)
    if(len(value) > t.size):
      raise SerializationError(f'value is {len(value)}, but must be no longer than {t.size}')
    #Pad the value with zeros
    value = value + b'\0'*(t.size-len(value))
    stream.write(value)
    return
  elif(t.name == "string"):
    if(len(value) >= t.size):
      raise SerializationError(f'value is {len(value)}, but must be no longer than {t.size}, with room for a null byte')
    #Pad the value with zeros
    value = value + b'\0'*(t.size-len(value))
    stream.write(value)
    return
  else:
    raise SerializationError(f"Unkown type: {t.name}")
  
  data = pystruct.pack(format_str, value)

  stream.write(data)
  

def _unpack_type(stream: BufferedIOBase, t: ProtoType, registry: Registry) -> Any:
  value: Any = None
  if is_primitive(t):
    value = _unpack_primitive_type(stream, t)
  else:
    cls = registry.get(t.name)
    if registry.is_enum(t.name):
      # Serialize the enums underlying type
      value = cls(_unpack_type(stream, registry.get(t.name)._desc.type, registry))
    elif registry.is_struct(t.name):
      value = cls.unpack(stream)
    else:
      raise SerializationError(f'{t.name} is not a primitive type, struct, or enum')

  return value

def _unpack_primitive_type(stream: BufferedIOBase, t: ProtoType) -> None:
  data: bytes = b''
  format_str: str = '='

  if(t.name == "bool"):
    format_str += "?"
  elif(t.name == "int8"):
    format_str += "b"
  elif(t.name == "uint8"):
    format_str += "B"
  elif(t.name == "int16"):
    format_str += "h"
  elif(t.name == "uint16"):
    format_str += "H"
  elif(t.name == "int32"):
    format_str += "i"
  elif(t.name == "uint32"):
    format_str += "I"
  elif(t.name == "int64"):
    format_str += "q"
  elif(t.name == "uint64"):
    format_str += "Q"
  elif(t.name == "float32"):
    format_str += "f"
  elif(t.name == "float64"):
    format_str += "d"
  elif(t.name == "bytes"):
    data = stream.read(t.size)
    return data
  elif(t.name == "string"):
    data = stream.read(t.size)
    return data.rstrip(b'\00') # Strip null bytes from string

  data = stream.read(pystruct.calcsize(format_str))
  print(data)
  value = pystruct.unpack(format_str, data)[0]

  return value

def pack(self, stream: BufferedIOBase) -> None:
  member: ProtoStructMember
  for member in self._desc.members:
    value = getattr(self, member.name)
    if member.arraySize is None:
      _pack_type(stream, value, member.type, self._registry)
    else:
      if len(value) != member.arraySize:
        raise SerializationError(f"Expected {t.size} elements in array, got {len(value)}")
      for element in value:
        _pack_type(stream, element, member.type, self._registry)


def unpack(cls, stream: BufferedIOBase) -> None:
  members = {}
  member: ProtoStructMember
  for member in cls._desc.members:
    if member.arraySize is None:
      members[member.name] = _unpack_type(stream, member.type, cls._registry)
    else:
      value = []
      for i in range(0, member.arraySize):
        value.append(_unpack_type(stream, member.type, cls._registry))
      members[member.name] = value
  
  return cls(**members)


class struct:
  def __init__(self, registry: Registry, json_desc: str):
    self.json_desc = json_desc
    self.registry = registry

  def __call__(self, cls):
    if not is_dataclass(cls):
      raise SerializationError(f'{cls} is not a dataclass')

    desc: ProtoStruct
    desc = ProtoStruct.from_json(self.json_desc)

    cls.pack = pack
    cls.unpack = partial(unpack, cls)
    cls._desc = desc
    cls._registry = self.registry

    self.registry.register(desc.name, cls)

    return cls


class enum:
  def __init__(self, registry: Registry, json_desc: str):
    self.json_desc = json_desc
    self.registry = registry

  def __call__(self, cls):
    if not issubclass(cls, Enum):
      raise SerializationError(f'{cls} is not a enum')

    desc: ProtoEnum
    desc = ProtoEnum.from_json(self.json_desc)

    cls._desc = desc
    cls._registry = self.registry

    self.registry.register(desc.name, cls)

    return cls