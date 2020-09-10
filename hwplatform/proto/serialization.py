from typing import Any
from dataclasses import is_dataclass, fields, field
from enum import Enum
from functools import partial
import bitstring
from bitstring import BitArray, BitStream

from .types import ProtoStruct, ProtoEnum, ProtoStructMember, ProtoType, is_primitive
from .runtime import Registry


class SerializationError(RuntimeError):
  pass

def _pack_type(value: Any, t: ProtoType, registry: Registry) -> BitStream:
  bits: BitStream = None

  if is_primitive(t):
    bits = _pack_primitive_type(value, t)
  elif t.array:
    bits = BitStream()
    if registry.is_enum(t.name):
      if len(value) != t.size:
        raise RuntimeError(f"Expected {t.size} elements in array, got {len(value)}")
      for v in value:
        bits += _pack_type(v.value, registry.get(t.name)._desc.type, registry)
    else:
      if len(value) != t.size:
        raise RuntimeError(f"Expected {t.size} elements in array, got {len(value)}")
      for v in value:
        bits += v.pack()
  else:
    if registry.is_enum(t.name):
      # Serialize the enums underlying type
      bits = _pack_type(value.value, registry.get(t.name)._desc.type, registry)
    elif registry.is_struct(t.name):
      bits = value.pack()
    else:
      raise SerializationError(f'{t.name} is not a primitive type, struct, or enum')

  return bits

def _pack_primitive_type(value: Any, t: ProtoType) -> BitStream:
  bits: BitStream = None
  format_str: str = ''

  if(t.name == "ACK"):
    return BitStream()
  elif(t.name == "flag"):
    format_str = "bool:1"
  elif(t.name == "int"):
    format_str = f"int:{t.size}"
  elif(t.name == "uint"):
    format_str = f"uint:{t.size}"
  elif(t.name == "float"):
    format_str = f"float:{t.size}"
  elif(t.name == "bits"):
    format_str = f"bits:{t.size}"
  elif(t.name == "bytes"):
    if(len(value) > t.size):
      raise RuntimeError(f'value is {len(value)}, but must be no longer than {t.size}')
    #Pad the value with zeros
    value = value + b'\0'*(t.size-len(value))
    return BitStream(bytes=value, length=t.size * 8)
  elif(t.name == "string"):
    value = value.encode('ascii')
    if(len(value) > t.size):
      raise RuntimeError(f'value is {len(value)}, but must be no longer than {t.size}')
    #Pad the value with zeros
    value = value + b'\0'*(t.size-len(value))
    return BitStream(bytes=value, length=t.size * 8)
  elif(t.name == "utf8string"):
    value = value.encode('UTF-8')  # utf-8
    if(len(value) > t.size):
      raise RuntimeError(f'value is {len(value)}, but must be no longer than {t.size}')
    #Pad the value with zeros
    value = value + b'\0'*(t.size-len(value))
    print(BitStream(bytes=value, length=t.size * 8).hex)
    return BitStream(bytes=value, length=t.size * 8)

  print(f'{t.name} {format_str}')
  bits = bitstring.pack(format_str, value)
  
  return bits

def _unpack_type(stream: BitStream, t: ProtoType, registry: Registry) -> Any:
  value: Any = None
  if is_primitive(t):
    value = _unpack_primitive_type(stream, t)
  elif t.array:
    value = []
    cls = registry.get(t.name)
    if registry.is_enum(t.name):
      for _ in range(0, t.size):
        value.append(cls(_unpack_type(stream, registry.get(t.name)._desc.type, registry)))
    else:
      for _ in range(0, t.size):
        value.append(cls.unpack(stream))
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

def _unpack_primitive_type(stream: BitStream, t: ProtoType) -> BitStream:
  format_str: str = ''

  if(t.name == "ACK"):
    return BitStream()
  elif(t.name == "flag"):
    format_str = "bool:1"
  elif(t.name == "int"):
    format_str = f"int:{t.size}"
  elif(t.name == "uint"):
    format_str = f"uint:{t.size}"
  elif(t.name == "float"):
    format_str = f"float:{t.size}"
  elif(t.name == "bits"):
    format_str = f"bits:{t.size}"
  elif(t.name == "bytes"):
    bits = stream.read(f"bits:{t.size*8}")
    return bits.tobytes()
  elif(t.name == "string"):
    bits = stream.read(f"bits:{t.size*8}")
    return bits.tobytes().rstrip(b'\x00').decode('ascii')
  elif(t.name == "utf8string"):
    bits = stream.read(f"bits:{t.size*8}")
    return bits.tobytes().rstrip(b'\x00').decode('UTF-8')

  print(f'{t.name} {format_str}')
  value = stream.read(format_str)

  return value

def pack(self) -> BitStream:
  bits = BitStream()

  member: ProtoStructMember
  for member in self._desc.members:
    bits += _pack_type(getattr(self, member.name), member.type, self._registry)

  return bits


def unpack(cls, stream: BitStream) -> None:
  members = {}
  member: ProtoStructMember
  for member in cls._desc.members:
    members[member.name] = _unpack_type(stream, member.type, cls._registry)
  
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