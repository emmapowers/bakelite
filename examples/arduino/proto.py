from dataclasses import dataclass
from enum import Enum
from bakelite.proto.serialization import struct, enum
from bakelite.proto.runtime import Registry, ProtocolBase
from typing import List






registry = Registry()




@struct(registry, r'''{"members": [{"type": {"name": "uint8", "size": null}, "name": "a", "value": null, "comment": null, "annotations": [], "arraySize": null}, {"type": {"name": "int32", "size": null}, "name": "b", "value": null, "comment": null, "annotations": [], "arraySize": null}, {"type": {"name": "bool", "size": null}, "name": "status", "value": null, "comment": null, "annotations": [], "arraySize": null}, {"type": {"name": "string", "size": 16}, "name": "message", "value": null, "comment": null, "annotations": [], "arraySize": null}], "name": "TestMessage", "comment": null, "annotations": []}''')
@dataclass
class TestMessage:
  a: int
  b: int
  status: bool
  message: str
  
  
@struct(registry, r'''{"members": [{"type": {"name": "uint8", "size": null}, "name": "code", "value": null, "comment": null, "annotations": [], "arraySize": null}, {"type": {"name": "string", "size": 128}, "name": "message", "value": null, "comment": null, "annotations": [], "arraySize": null}], "name": "Ack", "comment": null, "annotations": []}''')
@dataclass
class Ack:
  code: int
  message: str
  
  



class Protocol(ProtocolBase):
  def __init__(self, **kwargs):
    super().__init__(
      maxLength="256",
      framing="COBS",
      crc="None",
      registry=registry,
      desc=r'''{"options": [{"name": "maxLength", "value": "256", "comment": null, "annotations": []}, {"name": "framing", "value": "COBS", "comment": null, "annotations": []}, {"name": "crc", "value": "None", "comment": null, "annotations": []}], "message_ids": [{"name": "TestMessage", "number": 1, "comment": null, "annotations": []}, {"name": "Ack", "number": 2, "comment": null, "annotations": []}], "comment": null, "annotations": []}''',
      **kwargs
    )

