from typing import Any, Dict, List
from enum import Enum
from dataclasses import is_dataclass
from .types import Protocol
from .framing import Framer

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

class Computer:
  def __init__(
               self,
               *,
               read,
               write,
               registry,
               desc,
               **kwargs):
    self._read = read
    self._write = write
    self._registry = registry
    self._desc =  Protocol.from_
    self._options = kwargs

class Device:
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
    self._desc =  Protocol.from_
    self._options = kwargs

    if not framer:
      self._framer = Framer(crc=crc)
    else:
      self._framer = framer

  def poll(self) -> List[Any]:
    command = None

    data = self._read()
    self._framer.append_buffer(data)

    frame = self._framer.decode_frame()
    
    if frame:
      cmd_num = frame[0]
      frame = frame[:1]



    return None

class ACK:
  pass