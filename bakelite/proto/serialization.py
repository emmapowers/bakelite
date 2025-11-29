"""Serialization and deserialization for bakelite protocol types."""

import struct as pystruct
from dataclasses import is_dataclass
from enum import Enum
from io import BufferedIOBase
from typing import Any, Protocol, TypeVar, runtime_checkable

from .runtime import Registry
from .types import ProtoEnumDescriptor, ProtoStruct, ProtoStructMember, ProtoType


@runtime_checkable
class PackableProtocol(Protocol):
    """Protocol for types that can be packed/unpacked from binary streams."""

    _desc: ProtoStruct
    _registry: Registry

    def pack(self, stream: BufferedIOBase) -> None:
        """Pack this instance to a binary stream."""
        ...

    @classmethod
    def unpack(cls, stream: BufferedIOBase) -> "PackableProtocol":
        """Unpack an instance from a binary stream."""
        ...


@runtime_checkable
class EnumProtocol(Protocol):
    """Protocol for enum types registered with the protocol."""

    _desc: ProtoEnumDescriptor
    _registry: Registry
    value: Any


PRIMITIVE_TYPES = frozenset(
    {
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
    }
)


def is_primitive(t: ProtoType) -> bool:
    """Check if a type is a primitive type."""
    return t.name in PRIMITIVE_TYPES


class SerializationError(RuntimeError):
    """Raised when serialization or deserialization fails."""


def _pack_type(stream: BufferedIOBase, value: Any, t: ProtoType, registry: Registry) -> None:
    if is_primitive(t):
        _pack_primitive_type(stream, value, t)
    elif registry.is_enum(t.name):
        # Serialize the enum's underlying type
        enum_cls: type[EnumProtocol] = registry.get(t.name)
        _pack_type(stream, value.value, enum_cls._desc.type, registry)
    elif registry.is_struct(t.name):
        packable: PackableProtocol = value
        packable.pack(stream)
    else:
        raise SerializationError(f"{t.name} is not a primitive type, struct, or enum")


def _pack_primitive_type(stream: BufferedIOBase, value: Any, t: ProtoType) -> None:
    format_str: str = "="

    if t.name == "bool":
        format_str += "?"
    elif t.name == "int8":
        format_str += "b"
    elif t.name == "uint8":
        format_str += "B"
    elif t.name == "int16":
        format_str += "h"
    elif t.name == "uint16":
        format_str += "H"
    elif t.name == "int32":
        format_str += "i"
    elif t.name == "uint32":
        format_str += "I"
    elif t.name == "int64":
        format_str += "q"
    elif t.name == "uint64":
        format_str += "Q"
    elif t.name == "float32":
        format_str += "f"
    elif t.name == "float64":
        format_str += "d"
    elif t.name == "bytes" and t.size == 0:
        if not isinstance(value, bytes):
            raise RuntimeError(f"expected bytes object for field {t.name}")
        if len(value) > 255:
            raise SerializationError(f"value is {len(value)}, but must be no longer than 255")
        stream.write(pystruct.pack("=B", len(value)))
        stream.write(value)
        return
    elif t.name == "bytes":
        assert t.size is not None
        if len(value) > t.size:
            raise SerializationError(f"value is {len(value)}, but must be no longer than {t.size}")
        # Pad the value with zeros
        value = value + b"\0" * (t.size - len(value))
        stream.write(value)
        return
    elif t.name == "string" and t.size == 0:
        if value[:-1].find(b"\x00") > 0:
            raise SerializationError("Found a null byte before the end of the string")
        if value[-1] != 0:
            value = value + b"\0"
        stream.write(value)
        return
    elif t.name == "string":
        assert t.size is not None
        if not isinstance(value, bytes):
            raise SerializationError("string values must be encoded as bytes")
        if len(value) >= t.size:
            raise SerializationError(
                f"value is {len(value)}, but must be no longer than {t.size}, "
                "with room for a null byte"
            )
        # Pad the value with zeros
        value = value + b"\0" * (t.size - len(value))
        stream.write(value)
        return
    else:
        raise SerializationError(f"Unknown type: {t.name}")

    data = pystruct.pack(format_str, value)
    stream.write(data)


def _unpack_type(stream: BufferedIOBase, t: ProtoType, registry: Registry) -> Any:
    if is_primitive(t):
        return _unpack_primitive_type(stream, t)

    if registry.is_enum(t.name):
        # Deserialize the enum's underlying type
        # The class is an Enum with EnumProtocol attributes added by @enum decorator
        enum_cls: type[Enum] = registry.get(t.name)
        enum_proto: type[EnumProtocol] = registry.get(t.name)
        return enum_cls(_unpack_type(stream, enum_proto._desc.type, registry))
    if registry.is_struct(t.name):
        struct_cls: type[PackableProtocol] = registry.get(t.name)
        return struct_cls.unpack(stream)
    raise SerializationError(f"{t.name} is not a primitive type, struct, or enum")


def _unpack_primitive_type(stream: BufferedIOBase, t: ProtoType) -> Any:
    format_str: str = "="

    if t.name == "bool":
        format_str += "?"
    elif t.name == "int8":
        format_str += "b"
    elif t.name == "uint8":
        format_str += "B"
    elif t.name == "int16":
        format_str += "h"
    elif t.name == "uint16":
        format_str += "H"
    elif t.name == "int32":
        format_str += "i"
    elif t.name == "uint32":
        format_str += "I"
    elif t.name == "int64":
        format_str += "q"
    elif t.name == "uint64":
        format_str += "Q"
    elif t.name == "float32":
        format_str += "f"
    elif t.name == "float64":
        format_str += "d"
    elif t.name == "bytes" and t.size == 0:
        size = pystruct.unpack("=B", stream.read(1))[0]
        return stream.read(size)
    elif t.name == "bytes":
        return stream.read(t.size)
    elif t.name == "string" and t.size == 0:
        data = b""
        while True:
            byte = stream.read(1)
            if byte in {b"\x00", b""}:
                break
            data += byte
        return data
    elif t.name == "string":
        data = stream.read(t.size)
        # Return characters up until the null byte
        return data[: data.find(b"\00")]
    else:
        raise SerializationError(f"Unknown type: {t.name}")

    data = stream.read(pystruct.calcsize(format_str))
    return pystruct.unpack(format_str, data)[0]


def pack(self: Any, stream: BufferedIOBase) -> None:
    """Pack a struct instance to a binary stream."""
    member: ProtoStructMember
    for member in self._desc.members:
        value = getattr(self, member.name)
        if member.array_size is None:
            _pack_type(stream, value, member.type, self._registry)
        else:
            if member.array_size != 0:
                if len(value) != member.array_size:
                    raise SerializationError(
                        f"Expected {member.array_size} elements in array, got {len(value)}"
                    )
            else:
                if len(value) > 255:
                    raise SerializationError(
                        f"Got an array of size {len(value)}. Arrays must not exceed 255 elements"
                    )
                stream.write(pystruct.pack("=B", len(value)))
            for element in value:
                _pack_type(stream, element, member.type, self._registry)


TPackable = TypeVar("TPackable", bound="PackableProtocol")


def unpack(cls: type[TPackable], stream: BufferedIOBase) -> TPackable:
    """Unpack a struct instance from a binary stream."""
    members: dict[str, Any] = {}
    member: ProtoStructMember
    for member in cls._desc.members:
        if member.array_size is None:
            members[member.name] = _unpack_type(
                stream,
                member.type,
                cls._registry,
            )
        else:
            value = []
            size = member.array_size

            if size == 0:
                size = pystruct.unpack("=B", stream.read(1))[0]

            for _ in range(size):
                value.append(_unpack_type(stream, member.type, cls._registry))
            members[member.name] = value

    return cls(**members)


T = TypeVar("T")


class Packable:
    """Mixin base class for packable types. Use with @struct decorator."""

    _desc: ProtoStruct
    _registry: Registry

    def pack(self, stream: BufferedIOBase) -> None:
        """Pack this instance to a binary stream."""

    @classmethod
    def unpack(cls, stream: BufferedIOBase) -> "Packable":
        """Unpack an instance from a binary stream."""
        raise NotImplementedError


class struct:
    """Decorator that adds pack/unpack methods to a dataclass."""

    def __init__(self, registry: Registry, desc: ProtoStruct) -> None:
        self.desc = desc
        self.registry = registry

    def __call__(self, cls: type[T]) -> type[T]:
        if not is_dataclass(cls):
            raise SerializationError(f"{cls} is not a dataclass")

        # Add protocol attributes and methods to the class
        setattr(cls, "pack", pack)
        setattr(cls, "unpack", classmethod(lambda c, s: unpack(c, s)).__get__(None, cls))
        setattr(cls, "_desc", self.desc)
        setattr(cls, "_registry", self.registry)

        self.registry.register(self.desc.name, cls)

        return cls


TEnum = TypeVar("TEnum", bound=Enum)


class enum:
    """Decorator that registers an enum type with the protocol registry."""

    def __init__(self, registry: Registry, desc: ProtoEnumDescriptor) -> None:
        self.desc = desc
        self.registry = registry

    def __call__(self, cls: type[TEnum]) -> type[TEnum]:
        if not issubclass(cls, Enum):
            raise SerializationError(f"{cls} is not an enum")

        # Add protocol attributes to the enum class
        setattr(cls, "_desc", self.desc)
        setattr(cls, "_registry", self.registry)

        self.registry.register(self.desc.name, cls)

        return cls
