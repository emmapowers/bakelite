"""Protocol definition parser using Lark."""

import os
from dataclasses import dataclass
from typing import Any, TypeVar

from lark import Lark
from lark.visitors import Transformer

from .types import (
    ProtoAnnotation,
    ProtoAnnotationArg,
    Protocol,
    ProtoEnum,
    ProtoEnumValue,
    ProtoMessageId,
    ProtoOption,
    ProtoStruct,
    ProtoStructMember,
    ProtoType,
)

_g_parser: Lark | None = None


class ValidationError(RuntimeError):
    """Raised when protocol validation fails."""


@dataclass
class _Array:
    value: int


@dataclass
class _Comment:
    value: str


@dataclass
class _Name:
    value: str


@dataclass
class _Value:
    value: str


@dataclass
class _Number:
    value: int


@dataclass
class _ProtoMessageIds:
    ids: list[ProtoMessageId]


TFilter = TypeVar("TFilter", bound=object)


def _filter(args: list[Any], class_type: type[TFilter]) -> list[TFilter]:
    return [v for v in args if isinstance(v, class_type)]


def _find_one(args: list[Any], class_type: type[object]) -> Any:
    filtered = _filter(args, class_type)
    if len(filtered) == 0:
        return None
    if len(filtered) > 1:
        raise RuntimeError(f"Found more than one {class_type}")

    # Return _Array objects as-is (they have multiple fields we need)
    if isinstance(filtered[0], _Array):
        return filtered[0]

    if hasattr(filtered[0], "value"):
        return filtered[0].value
    return filtered[0]


TMany = TypeVar("TMany")


def _find_many(args: list[Any], class_type: type[TMany]) -> list[TMany]:
    return _filter(args, class_type)


class TreeTransformer(Transformer):
    """Transform parse tree into protocol types."""

    def array(self, args: list[Any]) -> _Array:
        return _Array(value=int(args[0]))

    def argument_val(self, args: list[Any]) -> ProtoAnnotationArg:
        if len(args) == 1:
            return ProtoAnnotationArg(name=None, value=str(args[0]))
        if len(args) == 2:
            return ProtoAnnotationArg(name=str(args[0]), value=str(args[1]))
        raise RuntimeError("Argument has more than three args")

    def annotation(self, args: list[Any]) -> ProtoAnnotation:
        return ProtoAnnotation(name=str(args[0]), arguments=_find_many(args, ProtoAnnotationArg))

    def comment(self, args: list[Any]) -> _Comment:
        return _Comment(value=str(args[0]))

    def enum(self, args: list[Any]) -> ProtoEnum:
        return ProtoEnum(
            type=_find_one(args, ProtoType),
            name=_find_one(args, _Name),
            values=_find_many(args, ProtoEnumValue),
            comment=_find_one(args, _Comment),
            annotations=_find_many(args, ProtoAnnotation),
        )

    def enum_value(self, args: list[Any]) -> ProtoEnumValue:
        return ProtoEnumValue(
            name=_find_one(args, _Name),
            value=_find_one(args, _Value),
            comment=_find_one(args, _Comment),
            annotations=_find_many(args, ProtoAnnotation),
        )

    def name(self, args: list[Any]) -> _Name:
        return _Name(value=str(args[0]))

    def number(self, args: list[Any]) -> _Number:
        return _Number(value=int(args[0]))

    def prim(self, args: list[Any]) -> ProtoType:
        return ProtoType(name=str(args[0]), size=None)

    def prim_sized(self, args: list[Any]) -> ProtoType:
        return ProtoType(name=str(args[0]), size=int(args[1]))

    def proto(self, args: list[Any]) -> Protocol:
        ids = _find_one(args, _ProtoMessageIds)

        return Protocol(
            options=_find_many(args, ProtoOption),
            message_ids=ids.ids if ids else [],
            comment=_find_one(args, _Comment),
            annotations=_find_many(args, ProtoAnnotation),
        )

    def proto_message_id(self, args: list[Any]) -> ProtoMessageId:
        return ProtoMessageId(
            name=_find_one(args, _Name),
            number=_find_one(args, _Number),
            comment=_find_one(args, _Comment),
            annotations=_find_many(args, ProtoAnnotation),
        )

    def proto_message_ids(self, args: list[Any]) -> _ProtoMessageIds:
        return _ProtoMessageIds(ids=_find_many(args, ProtoMessageId))

    def proto_member(self, args: list[Any]) -> ProtoOption:
        return ProtoOption(
            name=_find_one(args, _Name),
            value=_find_one(args, _Value),
            comment=_find_one(args, _Comment),
            annotations=_find_many(args, ProtoAnnotation),
        )

    def struct(self, args: list[Any]) -> ProtoStruct:
        return ProtoStruct(
            name=_find_one(args, _Name),
            members=_find_many(args, ProtoStructMember),
            comment=_find_one(args, _Comment),
            annotations=_find_many(args, ProtoAnnotation),
        )

    def struct_member(self, args: list[Any]) -> ProtoStructMember:
        array = _find_one(args, _Array)
        return ProtoStructMember(
            name=_find_one(args, _Name),
            type=_find_one(args, ProtoType),
            value=_find_one(args, _Value),
            comment=_find_one(args, _Comment),
            annotations=_find_many(args, ProtoAnnotation),
            array_size=array.value if array else None,
        )

    def value(self, args: list[Any]) -> _Value:
        return _Value(value=str(args[0]))

    def type(self, args: list[Any]) -> ProtoType:
        if isinstance(args[0], ProtoType):
            return args[0]
        return ProtoType(name=str(args[0]), size=None)


def validate(
    enums: list[ProtoEnum],
    structs: list[ProtoStruct],
    protocol: Protocol | None,
    _comments: list[str],
) -> None:
    """Validate parsed protocol definition."""
    _enum_map = {enum.name: enum for enum in enums}
    struct_map = {struct.name: struct for struct in structs}

    if not protocol:
        return

    # Validate Message IDs
    for msg_id in protocol.message_ids:
        if msg_id.number == 0:
            raise ValidationError("Message ID 0 is reserved for future use")
        if msg_id.name not in struct_map:
            raise ValidationError(f"{msg_id.name} assigned a message ID, but not declared")


def parse(
    text: str,
) -> tuple[list[ProtoEnum], list[ProtoStruct], Protocol | None, list[str]]:
    """Parse a protocol definition file."""
    global _g_parser

    if not _g_parser:
        with open(f"{os.path.dirname(__file__)}/protodef.lark", encoding="utf-8") as f:
            grammar = f.read()

        # Note: cache=True requires parser="lalr" which doesn't work with the
        # current grammar due to ambiguities. Grammar refactoring would be needed.
        _g_parser = Lark(grammar)

    tree = _g_parser.parse(text)
    tree = TreeTransformer().transform(tree)

    items = next(iter(tree.iter_subtrees_topdown())).children

    enums = _find_many(items, ProtoEnum)
    structs = _find_many(items, ProtoStruct)
    protocol = _find_one(items, Protocol)
    comments = [comment.value for comment in _find_many(items, _Comment)]

    validate(enums, structs, protocol, comments)

    return (enums, structs, protocol, comments)
