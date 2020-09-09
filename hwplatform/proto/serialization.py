from dataclasses import is_dataclass, fields, field
from enum import Enum
from bitstring import BitArray, BitStream

from .types import ProtoStruct, ProtoEnum, ProtoStructMember


class SerializationError(RuntimeError):
  pass


def pack(self) -> BitStream:
  bits = BitStream()

  member: ProtoStructMember
  for member in self._desc.members:
    bits += _pack_member(self[member.name], member)

  return bits


def unpack(self, data: BitStream) -> None:
  member: ProtoStructMember
  for member in self._desc.members:
    self[member.name] = _unpack_member(data, member)


class struct:
  def __init__(self, json_desc: str):
    self.json_desc = json_desc

  def __call__(self, cls):
    if not is_dataclass(cls):
      raise SerializationError(f'{cls} is not a dataclass')

    desc = ProtoStruct.from_json(self.json_desc)

    cls.pack = pack
    cls.unpack = unpack
    cls._desc = desc

    return cls


class enum:
  def __init__(self, json_desc: str):
    self.json_desc = json_desc

  def __call__(self, cls):
    if not issubclass(cls, Enum):
      raise SerializationError(f'{cls} is not a enum')

    desc = ProtoEnum.from_json(self.json_desc)

    cls._desc = desc

    return cls