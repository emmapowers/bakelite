"""Generated protocol definitions."""

import struct as _struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import ClassVar, Self

_runtime_path = str(Path(__file__).parent)
_added_to_path = _runtime_path not in sys.path
if _added_to_path:
    sys.path.insert(0, _runtime_path)
try:
    from bakelite_runtime.serialization import BakeliteEnum, SerializationError, Struct, bakelite_field
    from bakelite_runtime.runtime import ProtocolBase
finally:
    if _added_to_path:
        sys.path.remove(_runtime_path)


@dataclass
class TestMessage(Struct):
    a: int = bakelite_field(type="uint8")
    b: int = bakelite_field(type="int32")
    status: bool = bakelite_field(type="bool")
    message: str = bakelite_field(type="string", max_length=16)

    def pack(self) -> bytes:
        _buf = bytearray()
        _buf.extend(_struct.pack("=Bi?", self.a, self.b, self.status))
        _enc_message = self.message.encode("ascii")
        if len(_enc_message) >= 16:
            raise SerializationError("message exceeds 15 chars")
        _buf.extend(_enc_message)
        _buf.extend(b"\x00" * (16 - len(_enc_message)))
        return bytes(_buf)

    @classmethod
    def unpack(cls, _data: bytes | memoryview, offset: int = 0) -> tuple[Self, int]:
        _o = offset
        a, b, status = _struct.unpack_from("=Bi?", _data, _o)
        _o += 6
        _raw_message = _data[_o:_o + 16]
        _null_message = _raw_message.find(b"\x00")
        message = bytes(_raw_message[:_null_message if _null_message >= 0 else 16]).decode("ascii")
        _o += 16
        return cls(a, b, status, message), _o - offset


@dataclass
class Ack(Struct):
    code: int = bakelite_field(type="uint8")
    message: str = bakelite_field(type="string", max_length=64)

    def pack(self) -> bytes:
        _buf = bytearray()
        _buf.extend(_struct.pack("=B", self.code))
        _enc_message = self.message.encode("ascii")
        if len(_enc_message) >= 64:
            raise SerializationError("message exceeds 63 chars")
        _buf.extend(_enc_message)
        _buf.extend(b"\x00" * (64 - len(_enc_message)))
        return bytes(_buf)

    @classmethod
    def unpack(cls, _data: bytes | memoryview, offset: int = 0) -> tuple[Self, int]:
        _o = offset
        code, = _struct.unpack_from("=B", _data, _o)
        _o += 1
        _raw_message = _data[_o:_o + 64]
        _null_message = _raw_message.find(b"\x00")
        message = bytes(_raw_message[:_null_message if _null_message >= 0 else 64]).decode("ascii")
        _o += 64
        return cls(code, message), _o - offset


class Protocol(ProtocolBase):
    _message_types: ClassVar[dict[int, type[Struct]]] = {
        1: TestMessage,
        2: Ack,
    }
    _message_ids: ClassVar[dict[str, int]] = {
        "TestMessage": 1,
        "Ack": 2,
    }

    def __init__(self, **kwargs) -> None:
        kwargs.setdefault("crc", "CRC8")
        super().__init__(**kwargs)
