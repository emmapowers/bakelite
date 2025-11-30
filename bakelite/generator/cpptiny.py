"""C++ (tiny) code generator for bakelite protocols."""

import os
from copy import copy

from jinja2 import Environment, PackageLoader

from .types import Protocol, ProtoEnum, ProtoStruct, ProtoStructMember, ProtoType

env = Environment(
    loader=PackageLoader("bakelite.generator", "templates"),
    trim_blocks=True,
    lstrip_blocks=True,
    keep_trailing_newline=True,
    line_comment_prefix="%%",
    line_statement_prefix="%",
)

template = env.get_template("cpptiny.h.j2")

PRIMITIVE_TYPE_MAP = {
    "bool": "bool",
    "int8": "int8_t",
    "int16": "int16_t",
    "int32": "int32_t",
    "int64": "int64_t",
    "uint8": "uint8_t",
    "uint16": "uint16_t",
    "uint32": "uint32_t",
    "uint64": "uint64_t",
    "float32": "float",
    "float64": "double",
    "bytes": "char",
    "string": "char",
}


def _map_type(t: ProtoType) -> str:
    return PRIMITIVE_TYPE_MAP.get(t.name, t.name)


def _map_type_member(member: ProtoStructMember) -> str:
    type_name = _map_type(member.type)

    if member.type.name == "bytes" and member.type.size == 0 and member.array_size == 0:
        return f"Bakelite::SizedArray<Bakelite::SizedArray<{type_name}> >"
    if member.type.name == "bytes" and member.type.size == 0:
        return f"Bakelite::SizedArray<{type_name}>"
    if member.type.name == "string" and (member.type.size == 0) and member.array_size == 0:
        return f"Bakelite::SizedArray<{type_name}*>"
    if member.type.name == "string" and (member.type.size == 0):
        return f"{type_name}*"
    if member.array_size == 0:
        return f"Bakelite::SizedArray<{type_name}>"
    return type_name


def _size_postfix(member: ProtoStructMember) -> str:
    if member.type.name in {"bytes", "string"}:
        if member.type.size == 0:
            return ""
        return f"[{member.type.size}]"
    return ""


def _array_postfix(member: ProtoStructMember) -> str:
    if member.array_size is None or member.array_size == 0:
        return ""
    return f"[{member.array_size}]"


def overhead(size: int, crc_size: int) -> int:
    """Calculate COBS overhead for a message size."""
    cobs_overhead = int((size + 253) / 254)
    return cobs_overhead + crc_size + 1


def render(
    enums: list[ProtoEnum],
    structs: list[ProtoStruct],
    proto: Protocol | None,
    comments: list[str],
    *,
    unpacked: bool = False,
) -> str:
    """Render a protocol definition to C++ source code.

    Args:
        enums: Enum definitions from the protocol
        structs: Struct definitions from the protocol
        proto: Protocol definition (framing, CRC, message IDs)
        comments: Top-level comments from the protocol file
        unpacked: If True, generate aligned structs with memmove shuffle.
                  If False (default), generate packed structs for zero-copy.
    """
    enums_types = {enum.name: enum for enum in enums}
    structs_types = {struct.name: struct for struct in structs}

    def _write_type(member: ProtoStructMember) -> str:
        if member.array_size is not None:
            size_arg = f", {member.array_size}" if member.array_size > 0 else ""
            tmp_member = copy(member)
            tmp_member.array_size = None
            tmp_member.name = "val"
            return f"""writeArray(stream, {member.name}{size_arg}, [](T &stream, const auto &val) {{
      return {_write_type(tmp_member)}
    }});"""
        if member.type.name in enums_types:
            underlying_type = _map_type(enums_types[member.type.name].type)
            return f"write(stream, ({underlying_type}){member.name});"
        if member.type.name in structs_types:
            return f"{member.name}.pack(stream);"
        if member.type.name in PRIMITIVE_TYPE_MAP and member.type.name not in {"bytes", "string"}:
            return f"write(stream, {member.name});"
        if member.type.name == "bytes":
            if member.type.size != 0:
                return f"writeBytes(stream, {member.name}, {member.type.size});"
            return f"writeBytes(stream, {member.name});"
        if member.type.name == "string":
            if member.type.size != 0:
                return f"writeString(stream, {member.name}, {member.type.size});"
            return f"writeString(stream, {member.name});"
        raise RuntimeError(f"Unknown type {member.type.name}")

    def _read_type(member: ProtoStructMember) -> str:
        if member.array_size is not None:
            size_arg = f", {member.array_size}" if member.array_size > 0 else ""
            tmp_member = copy(member)
            tmp_member.array_size = None
            tmp_member.name = "val"
            return f"""readArray(stream, {member.name}{size_arg}, [](T &stream, auto &val) {{
      return {_read_type(tmp_member)}
    }});"""
        if member.type.name in enums_types:
            underlying_type = _map_type(enums_types[member.type.name].type)
            return f"read(stream, ({underlying_type}&){member.name});"
        if member.type.name in structs_types:
            return f"{member.name}.unpack(stream);"
        if member.type.name in PRIMITIVE_TYPE_MAP and member.type.name not in {"bytes", "string"}:
            return f"read(stream, {member.name});"
        if member.type.name == "bytes":
            if member.type.size != 0:
                return f"readBytes(stream, {member.name}, {member.type.size});"
            return f"readBytes(stream, {member.name});"
        if member.type.name == "string":
            if member.type.size != 0:
                return f"readString(stream, {member.name}, {member.type.size});"
            return f"readString(stream, {member.name});"
        raise RuntimeError(f"Unknown type {member.type.name}")

    message_ids: list[tuple[str, int]] = []
    framer = ""

    if proto is not None:
        message_ids = [(msg.name, msg.number) for msg in proto.message_ids]
        options = {option.name: option.value for option in proto.options}
        crc = options.get("crc", "none").lower()
        framing = options.get("framing", "").lower()
        max_length = options.get("maxLength")

        if framing == "":
            raise RuntimeError("A frame type must be specified")

        if max_length is None:
            raise RuntimeError("maxLength must be specified")

        if crc == "none":
            crc_type = "CrcNoop"
            crc_size = 0
        elif crc == "crc8":
            crc_type = "Crc8"
            crc_size = 1
        elif crc == "crc16":
            crc_type = "Crc16"
            crc_size = 2
        elif crc == "crc32":
            crc_type = "Crc32"
            crc_size = 4
        else:
            raise RuntimeError(f"Unknown CRC type {crc}")

        max_length = int(max_length)
        max_length += overhead(int(max_length), crc_size)

        if framing == "cobs":
            framer = f"Bakelite::CobsFramer<Bakelite::{crc_type}, {max_length}>"
        else:
            raise RuntimeError(f"Unknown framing type {framing}")

    return template.render(
        enums=enums,
        structs=structs,
        proto=proto,
        comments=comments,
        map_type=_map_type,
        map_type_member=_map_type_member,
        array_postfix=_array_postfix,
        size_postfix=_size_postfix,
        write_type=_write_type,
        read_type=_read_type,
        framer=framer,
        message_ids=message_ids,
        unpacked=unpacked,
    )


def runtime() -> str:
    """Generate the C++ runtime support code."""

    def include(filename: str) -> str:
        with open(
            os.path.join(os.path.dirname(__file__), "runtimes", "cpptiny", filename),
            encoding="utf-8",
        ) as f:
            return f.read()

    runtime_template = env.get_template("cpptiny-bakelite.h.j2")
    return runtime_template.render(include=include)
