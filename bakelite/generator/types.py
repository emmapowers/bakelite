"""Type definitions for protocol parsing and code generation."""

from dataclasses import dataclass
from typing import Any

from dataclasses_json import DataClassJsonMixin


@dataclass
class ProtoType(DataClassJsonMixin):
    """Represents a primitive or user-defined type.

    For bytes/string types:
    - size=N: variable length with max capacity N
    - size=None: user-defined type (struct/enum) or fixed primitive
    """

    name: str
    size: int | None


@dataclass
class ProtoAnnotationArg(DataClassJsonMixin):
    """Represents an argument to an annotation."""

    name: str | None
    value: Any


@dataclass
class ProtoAnnotation(DataClassJsonMixin):
    """Represents an annotation on a protocol element."""

    name: str
    arguments: list[ProtoAnnotationArg]


@dataclass
class ProtoEnumValue(DataClassJsonMixin):
    """Represents a single enum value."""

    name: str
    value: Any
    comment: str | None
    annotations: list[ProtoAnnotation]


@dataclass
class ProtoEnum(DataClassJsonMixin):
    """Represents an enum type definition."""

    values: list[ProtoEnumValue]
    type: ProtoType
    name: str
    comment: str | None
    annotations: list[ProtoAnnotation]


@dataclass
class ProtoStructMember(DataClassJsonMixin):
    """Represents a member of a struct.

    For arrays:
    - array_size=N: variable length array with max capacity N
    - array_size=None: not an array
    """

    type: ProtoType
    name: str
    value: Any | None
    comment: str | None
    annotations: list[ProtoAnnotation]
    array_size: int | None


@dataclass
class ProtoStruct(DataClassJsonMixin):
    """Represents a struct type definition."""

    members: list[ProtoStructMember]
    name: str
    comment: str | None
    annotations: list[ProtoAnnotation]


@dataclass
class ProtoOption(DataClassJsonMixin):
    """Represents a protocol option."""

    name: str
    value: Any
    comment: str | None
    annotations: list[ProtoAnnotation]


@dataclass
class ProtoMessageId(DataClassJsonMixin):
    """Represents a message ID assignment."""

    name: str
    number: int
    comment: str | None
    annotations: list[ProtoAnnotation]


@dataclass
class Protocol(DataClassJsonMixin):
    """Represents a complete protocol definition."""

    options: list[ProtoOption]
    message_ids: list[ProtoMessageId]
    comment: str | None
    annotations: list[ProtoAnnotation]


PRIMITIVE_TYPES = frozenset(
    [
        "bool",
        "int8",
        "int16",
        "int32",
        "int64",
        "uint8",
        "uint16",
        "uint32",
        "uint64",
        "float32",
        "float64",
        "bytes",
        "string",
    ]
)


def primitive_types() -> list[str]:
    """Return a list of primitive type names."""
    return list(PRIMITIVE_TYPES)


def is_primitive(t: ProtoType) -> bool:
    """Check if a type is a primitive type."""
    return t.name in PRIMITIVE_TYPES
