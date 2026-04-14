"""Runtime type descriptors for bakelite protocol serialization.

These dataclasses describe the structure of protocol types at runtime,
used by the serialization decorators to implement pack/unpack methods.
"""

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class ProtoType:
    """Describes a primitive or user-defined type."""

    name: str
    size: int | None = None


@dataclass(frozen=True, slots=True)
class ProtoStructMember:
    """Describes a member of a struct."""

    type: ProtoType
    name: str
    array_size: int | None = None


@dataclass(frozen=True, slots=True)
class ProtoStruct:
    """Describes a struct type with its members."""

    name: str
    members: tuple[ProtoStructMember, ...]


@dataclass(frozen=True, slots=True)
class ProtoEnumDescriptor:
    """Describes an enum type."""

    name: str
    type: ProtoType


@dataclass(frozen=True, slots=True)
class ProtoMessageId:
    """Maps a message name to its numeric ID."""

    name: str
    number: int


@dataclass(frozen=True, slots=True)
class ProtocolDescriptor:
    """Describes a protocol with its message ID mappings."""

    message_ids: tuple[ProtoMessageId, ...]
