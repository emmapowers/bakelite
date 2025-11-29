"""Generated protocol definitions."""

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

_runtime_path = str(Path(__file__).parent)
_added_to_path = _runtime_path not in sys.path
if _added_to_path:
    sys.path.insert(0, _runtime_path)
try:
    from bakelite_runtime.runtime import ProtocolBase, Registry
    from bakelite_runtime.serialization import struct
    from bakelite_runtime.types import (
        ProtocolDescriptor,
        ProtoMessageId,
        ProtoStruct,
        ProtoStructMember,
        ProtoType,
    )
finally:
    if _added_to_path:
        sys.path.remove(_runtime_path)

registry = Registry()


@struct(
    registry,
    ProtoStruct(
        name="TestMessage",
        members=(
            ProtoStructMember(
                type=ProtoType(name="uint8", size=0),
                name="a",
            ),
            ProtoStructMember(
                type=ProtoType(name="int32", size=0),
                name="b",
            ),
            ProtoStructMember(
                type=ProtoType(name="bool", size=0),
                name="status",
            ),
            ProtoStructMember(
                type=ProtoType(name="string", size=16),
                name="message",
            ),
        ),
    ),
)
@dataclass
class TestMessage:
    a: int
    b: int
    status: bool
    message: str


@struct(
    registry,
    ProtoStruct(
        name="Ack",
        members=(
            ProtoStructMember(
                type=ProtoType(name="uint8", size=0),
                name="code",
            ),
            ProtoStructMember(
                type=ProtoType(name="string", size=64),
                name="message",
            ),
        ),
    ),
)
@dataclass
class Ack:
    code: int
    message: str


class Protocol(ProtocolBase):
    def __init__(self, **kwargs: Any) -> None:
        super().__init__(
            maxLength="70",
            framing="COBS",
            crc="CRC8",
            registry=registry,
            desc=ProtocolDescriptor(
                message_ids=(
                    ProtoMessageId(name="TestMessage", number=1),
                    ProtoMessageId(name="Ack", number=2),
                ),
            ),
            **kwargs,
        )
