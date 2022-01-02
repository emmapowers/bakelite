import struct
from dataclasses import is_dataclass
from enum import Enum
from io import BufferedIOBase, BytesIO
from typing import Any, Dict

from ..generator.types import Protocol
from .framing import CrcSize, Framer


class ProtocolError(RuntimeError):
  pass


class Registry:
  def __init__(self):
    self.types: Dict[str, Any] = {}

  def register(self, name: str, cls: Any) -> None:
    self.types[name] = cls

  def get(self, name: str):
    return self.types[name]

  def is_enum(self, name: str):
    return issubclass(self.types[name], Enum)

  def is_struct(self, name: str):
    return is_dataclass(self.types[name])


class ProtocolBase:
  _stream: BufferedIOBase
  _registry: Registry
  _desc: Protocol
  _options: Dict[str, Any]

  def __init__(self, *, stream, registry, desc, crc="CRC8", framer=None, **kwargs):
    self._stream = stream
    self._registry = registry
    self._desc = Protocol.from_json(desc)
    self._options = kwargs

    self._ids = {id.number: id.name for id in self._desc.message_ids}
    self._messages = {id.name: id.number for id in self._desc.message_ids}

    crc_size = CrcSize.NO_CRC
    crc = crc.lower()

    if crc == "none":
      crc_size = CrcSize.NO_CRC
    elif crc == "crc8":
      crc_size = CrcSize.CRC8
    elif crc == "crc16":
      crc_size = CrcSize.CRC16
    elif crc == "crc32":
      crc_size = CrcSize.CRC32
    else:
      raise RuntimeError(f"Unkown CRC type {crc}")

    if not framer:
      self._framer = Framer(crc=crc_size)
    else:
      self._framer = framer

  def send(self, message):
    if not getattr(message, "_desc"):
      raise ProtocolError(f"{type(message)} is not a message type")

    msg_name = message._desc.name
    if msg_name not in self._messages:
      raise ProtocolError(
          f"{type(message)} has not been assigned a message ID")
    msg_id = self._messages[msg_name]

    stream = BytesIO()
    stream.write(struct.pack("=B", msg_id))
    message.pack(stream)
    frame = self._framer.encode_frame(stream.getvalue())

    self._stream.write(frame)

  def poll(self) -> Any:
    data = self._stream.read()
    self._framer.append_buffer(data)

    frame = self._framer.decode_frame()

    if frame:
      msg_id = frame[0]
      msg = frame[1:]

      if msg_id in self._ids:
        message_type = self._registry.get(self._ids[msg_id])
        return message_type.unpack(BytesIO(msg))
      else:
        raise ProtocolError(f"Received unkown message id {msg_id}")

    return None
