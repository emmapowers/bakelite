"""C (tiny) code generator for bakelite protocols."""

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

template = env.get_template("ctiny.h.j2")

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
    "bytes": "uint8_t",
    "string": "char",
}

# Map primitive type names to serializer function suffixes
SERIALIZER_SUFFIX_MAP = {
    "bool": "bool",
    "int8": "int8",
    "int16": "int16",
    "int32": "int32",
    "int64": "int64",
    "uint8": "uint8",
    "uint16": "uint16",
    "uint32": "uint32",
    "uint64": "uint64",
    "float32": "float32",
    "float64": "float64",
}


def _map_type(t: ProtoType) -> str:
    return PRIMITIVE_TYPE_MAP.get(t.name, t.name)


def _map_type_member(member: ProtoStructMember) -> str:
    type_name = _map_type(member.type)

    # Variable-length bytes array
    if member.type.name == "bytes" and member.type.size == 0 and member.array_size == 0:
        return "Bakelite_SizedArray"  # Array of SizedArrays not supported yet
    if member.type.name == "bytes" and member.type.size == 0:
        return "Bakelite_SizedArray"
    # Variable-length string array
    if member.type.name == "string" and member.type.size == 0 and member.array_size == 0:
        return "Bakelite_SizedArray"  # Array of strings not supported yet
    if member.type.name == "string" and member.type.size == 0:
        return "char*"
    # Variable-length typed array
    if member.array_size == 0:
        return "Bakelite_SizedArray"
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
    """Render a protocol definition to C source code.

    Args:
        enums: Enum definitions from the protocol
        structs: Struct definitions from the protocol
        proto: Protocol definition (framing, CRC, message IDs)
        comments: Top-level comments from the protocol file
        unpacked: If True, generate aligned structs.
                  If False (default), generate packed structs for zero-copy.
    """
    enums_types = {enum.name: enum for enum in enums}
    structs_types = {struct.name: struct for struct in structs}

    def _write_type(member: ProtoStructMember) -> str:
        if member.array_size is not None:
            # Array handling
            if member.array_size > 0:
                # Fixed-size array
                tmp_member = copy(member)
                tmp_member.array_size = None
                tmp_member.name = f"self->{member.name}[i]"
                inner_write = _write_type(tmp_member)
                return f"""for (uint32_t i = 0; i < {member.array_size}; i++) {{
      if ((rcode = {inner_write}) != 0) return rcode;
    }}
    rcode = 0"""
            # Variable-length array
            tmp_member = copy(member)
            tmp_member.array_size = None
            tmp_member.name = f"(({_map_type(member.type)}*)self->{member.name}.data)[i]"
            inner_write = _write_type(tmp_member)
            return f"""bakelite_write_uint8(buf, self->{member.name}.size);
    for (uint8_t i = 0; i < self->{member.name}.size; i++) {{
      if ((rcode = {inner_write}) != 0) return rcode;
    }}
    rcode = 0"""

        # Use self-> prefix if not already present (for nested arrays, name includes self->)
        name = member.name if member.name.startswith("self->") else f"self->{member.name}"

        if member.type.name in enums_types:
            underlying = SERIALIZER_SUFFIX_MAP[enums_types[member.type.name].type.name]
            return f"bakelite_write_{underlying}(buf, ({_map_type(enums_types[member.type.name].type)}){name})"
        if member.type.name in structs_types:
            return f"{member.type.name}_pack(&{name}, buf)"
        if member.type.name in SERIALIZER_SUFFIX_MAP:
            suffix = SERIALIZER_SUFFIX_MAP[member.type.name]
            return f"bakelite_write_{suffix}(buf, {name})"
        if member.type.name == "bytes":
            if member.type.size != 0:
                return f"bakelite_write_bytes_fixed(buf, {name}, {member.type.size})"
            return f"bakelite_write_bytes(buf, &{name})"
        if member.type.name == "string":
            if member.type.size != 0:
                return f"bakelite_write_string_fixed(buf, {name}, {member.type.size})"
            return f"bakelite_write_string(buf, {name})"
        raise RuntimeError(f"Unknown type {member.type.name}")

    def _read_type(member: ProtoStructMember) -> str:
        if member.array_size is not None:
            # Array handling
            if member.array_size > 0:
                # Fixed-size array
                tmp_member = copy(member)
                tmp_member.array_size = None
                tmp_member.name = f"self->{member.name}[i]"
                inner_read = _read_type(tmp_member)
                return f"""for (uint32_t i = 0; i < {member.array_size}; i++) {{
      if ((rcode = {inner_read}) != 0) return rcode;
    }}
    rcode = 0"""
            # Variable-length array
            tmp_member = copy(member)
            tmp_member.array_size = None
            tmp_member.name = f"(({_map_type(member.type)}*)self->{member.name}.data)[i]"
            inner_read = _read_type(tmp_member)
            return f"""{{
      uint8_t arr_size;
      if ((rcode = bakelite_read_uint8(buf, &arr_size)) != 0) return rcode;
      self->{member.name}.size = arr_size;
      self->{member.name}.data = bakelite_buffer_alloc(buf, arr_size * sizeof({_map_type(member.type)}));
      if (self->{member.name}.data == NULL) return BAKELITE_ERR_ALLOC;
      for (uint8_t i = 0; i < arr_size; i++) {{
        if ((rcode = {inner_read}) != 0) return rcode;
      }}
    }}
    rcode = 0"""

        # Use self-> prefix if not already present (for nested arrays, name includes self->)
        name = member.name if member.name.startswith("self->") else f"self->{member.name}"

        if member.type.name in enums_types:
            underlying = SERIALIZER_SUFFIX_MAP[enums_types[member.type.name].type.name]
            underlying_type = _map_type(enums_types[member.type.name].type)
            return f"bakelite_read_{underlying}(buf, ({underlying_type}*)&{name})"
        if member.type.name in structs_types:
            return f"{member.type.name}_unpack(&{name}, buf)"
        if member.type.name in SERIALIZER_SUFFIX_MAP:
            suffix = SERIALIZER_SUFFIX_MAP[member.type.name]
            return f"bakelite_read_{suffix}(buf, &{name})"
        if member.type.name == "bytes":
            if member.type.size != 0:
                return f"bakelite_read_bytes_fixed(buf, {name}, {member.type.size})"
            return f"bakelite_read_bytes(buf, &{name})"
        if member.type.name == "string":
            if member.type.size != 0:
                return f"bakelite_read_string_fixed(buf, {name}, {member.type.size})"
            return f"bakelite_read_string(buf, &{name})"
        raise RuntimeError(f"Unknown type {member.type.name}")

    message_ids: list[tuple[str, int]] = []
    crc_type = "BAKELITE_CRC_NONE"
    crc_size = 0
    max_length = 0

    if proto is not None:
        message_ids = [(msg.name, msg.number) for msg in proto.message_ids]
        options = {option.name: option.value for option in proto.options}
        crc = options.get("crc", "none").lower()
        framing = options.get("framing", "").lower()
        max_length_opt = options.get("maxLength")

        if framing == "":
            raise RuntimeError("A frame type must be specified")

        if max_length_opt is None:
            raise RuntimeError("maxLength must be specified")

        if crc == "none":
            crc_type = "BAKELITE_CRC_NONE"
            crc_size = 0
        elif crc == "crc8":
            crc_type = "BAKELITE_CRC_8"
            crc_size = 1
        elif crc == "crc16":
            crc_type = "BAKELITE_CRC_16"
            crc_size = 2
        elif crc == "crc32":
            crc_type = "BAKELITE_CRC_32"
            crc_size = 4
        else:
            raise RuntimeError(f"Unknown CRC type {crc}")

        max_length = int(max_length_opt)

        if framing != "cobs":
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
        crc_type=crc_type,
        crc_size=crc_size,
        max_length=max_length,
        message_ids=message_ids,
        unpacked=unpacked,
    )


def runtime() -> str:
    """Generate the C runtime support code."""

    def include(filename: str) -> str:
        with open(
            os.path.join(os.path.dirname(__file__), "runtimes", "ctiny", filename),
            encoding="utf-8",
        ) as f:
            return f.read()

    runtime_template = env.get_template("ctiny-bakelite.h.j2")
    return runtime_template.render(include=include)
