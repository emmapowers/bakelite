"""Serialization and deserialization for bakelite protocol types."""

from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Self


class SerializationError(RuntimeError):
    """Raised when serialization or deserialization fails."""


@dataclass(frozen=True)
class BakeliteFieldInfo:
    """Metadata for a bakelite struct field."""

    bakelite_type: str
    max_length: int | None = None
    array_size: int | None = None  # None = not array, 0 = variable length, N = fixed


# Sentinel for missing default
_MISSING: Any = object()


def bakelite_field(
    type: str,
    *,
    max_length: int | None = None,
    array_size: int | None = None,
    default: Any = _MISSING,
    default_factory: Any = _MISSING,
) -> Any:
    """Define a bakelite field with serialization metadata.

    Args:
        type: The bakelite wire type (e.g., "uint8", "int32", "string").
        max_length: Maximum length for string/bytes types.
        array_size: Array size (None=not array, 0=variable, N=fixed).
        default: Default value for the field.
        default_factory: Factory function for default value.

    Returns:
        A dataclass field with bakelite metadata attached.
    """
    metadata = {"bakelite": BakeliteFieldInfo(type, max_length, array_size)}

    if default is not _MISSING:
        return field(default=default, metadata=metadata)
    if default_factory is not _MISSING:
        return field(default_factory=default_factory, metadata=metadata)
    return field(metadata=metadata)


class Struct:
    """Base class for generated struct types.

    Subclasses should be @dataclass decorated and define fields using
    bakelite_field() or type annotations for nested structs.

    Example:
        @dataclass
        class MyMessage(Struct):
            value: int = bakelite_field(type="uint8")
            name: str = bakelite_field(type="string", max_length=16)
            nested: OtherStruct  # no bakelite_field needed for structs
    """

    def pack(self) -> bytes:
        """Pack this struct to bytes. Generated code overrides this."""
        raise NotImplementedError("pack() must be implemented by generated code")

    @classmethod
    def unpack(cls, data: bytes | memoryview, offset: int = 0) -> tuple[Self, int]:
        """Unpack a struct from bytes.

        Args:
            data: The bytes to unpack from.
            offset: Starting offset in data.

        Returns:
            Tuple of (instance, bytes_consumed).
        """
        raise NotImplementedError("unpack() must be implemented by generated code")


class BakeliteEnum(Enum):
    """Base class for protocol enums.

    Subclasses specify wire type via class attribute:

    Example:
        class Status(BakeliteEnum):
            bakelite_type: ClassVar[str] = "uint8"
            OK = auto()
            ERROR = auto()
    """

    def pack(self) -> bytes:
        """Pack enum value. Generated code overrides this."""
        raise NotImplementedError("pack() must be implemented by generated code")

    @classmethod
    def unpack(cls, data: bytes | memoryview, offset: int = 0) -> tuple[Self, int]:
        """Unpack enum from bytes.

        Args:
            data: The bytes to unpack from.
            offset: Starting offset in data.

        Returns:
            Tuple of (enum member, bytes_consumed).
        """
        raise NotImplementedError("unpack() must be implemented by generated code")
