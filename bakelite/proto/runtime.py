from typing import Any, Dict, List
from enum import Enum
from dataclasses import is_dataclass
from bitstring import pack, BitStream

from .types import Protocol, ProtoMessageId, ProtoStruct
from .framing import Framer

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
  def __init__(
               self,
               *,
               read,
               write,
               registry,
               desc,
               crc=True,
               framer=None,
               **kwargs):
    self._read = read
    self._write = write
    self._registry = registry
    self._desc: List[ProtoMessageId]
    self._desc =  Protocol.from_json(desc)
    self._options = kwargs

    self._ids = {id.number: id.name for id in self._desc.message_ids}
    self._messages = {id.name: id.number for id in self._desc.message_ids}

    if not framer:
      self._framer = Framer(crc=crc)
    else:
      self._framer = framer

  def send(self, message):
    if not getattr(message, "_desc"):
      raise ProtocolError(f"{type(message)} is not a message type")
    
    msg_name = message._desc.name
    if msg_name not in self._messages:
      raise ProtocolError(f"{type(message)} has not been assigned a message ID")
    msg_id = self._messages[msg_name]

    stream = pack("uint:8", msg_id)
    stream += message.pack()
    frame = self._framer.encode_frame(stream.bytes)

    self._write(frame)

  def poll(self) -> List[Any]:
    command = None

    data = self._read()
    self._framer.append_buffer(data)

    frame = self._framer.decode_frame()
    
    if frame:
      msg_id = frame[0]
      msg = frame[1:]

      if msg_id in self._ids:
        message_type = self._registry.get(self._ids[msg_id])
        return message_type.unpack(BitStream(msg))
      else:
        raise ProtocolError(f"Received unkown message id {msg_id}")

    return None
