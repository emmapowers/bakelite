"""C (tiny) code generator for bakelite protocols."""

import os
from copy import copy

from jinja2 import Environment, PackageLoader

from .sizes import calculate_sizes
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
    """Map bakelite type to C type (for primitives and user types)."""
    return PRIMITIVE_TYPE_MAP.get(t.name, t.name)


def _map_type_member(member: ProtoStructMember) -> str:
    """Map struct member to C type declaration.

    For bytes[N] and T[N] arrays, this returns 'struct' which will be
    followed by the inline struct definition in the template.
    For string[N] (no array), returns 'char'.
    """
    # bytes[N] -> inline struct { uint8_t data[N]; uint8_t len; }
    if member.type.name == "bytes" and member.type.size is not None:
        return "struct"

    # string[N] without array -> char[N+1]
    # string[N][M] (array of strings) -> inline struct
    if member.type.name == "string" and member.type.size is not None:
        if member.array_size is not None:
            return "struct"
        return "char"

    # T[N] arrays -> inline struct { T data[N]; uint8_t len; }
    if member.array_size is not None:
        return "struct"

    # Primitives and user types
    return _map_type(member.type)


def _size_postfix(member: ProtoStructMember) -> str:
    """Get size suffix for the member declaration."""
    # For string[N], add [N+1] for null terminator
    if member.type.name == "string" and member.type.size is not None and member.array_size is None:
        return f"[{member.type.size + 1}]"
    return ""


def _inline_struct_def(member: ProtoStructMember) -> str:
    """Generate inline struct definition for bytes[N] or T[N] arrays."""
    # bytes[N] -> { uint8_t data[N]; uint8_t len; }
    if member.type.name == "bytes" and member.type.size is not None and member.array_size is None:
        return f"{{ uint8_t data[{member.type.size}]; uint8_t len; }}"

    # T[N] arrays
    if member.array_size is not None:
        # Special case for string arrays: char data[N][M+1]
        if member.type.name == "string" and member.type.size is not None:
            return f"{{ char data[{member.array_size}][{member.type.size + 1}]; uint8_t len; }}"
        inner_type = _get_element_type(member.type)
        return f"{{ {inner_type} data[{member.array_size}]; uint8_t len; }}"

    return ""


def _get_element_type(t: ProtoType) -> str:
    """Get C type for array elements."""
    if t.name == "bytes" and t.size is not None:
        return f"struct {{ uint8_t data[{t.size}]; uint8_t len; }}"
    if t.name == "string" and t.size is not None:
        return f"char[{t.size + 1}]"
    return _map_type(t)


def _array_postfix(member: ProtoStructMember) -> str:
    """Get array postfix (no longer used for variable arrays)."""
    return ""


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
    """Render a protocol definition to C source code."""
    enums_types = {enum.name: enum for enum in enums}
    structs_types = {struct.name: struct for struct in structs}

    def _write_type(member: ProtoStructMember) -> str:
        # Handle arrays (always variable-length now)
        if member.array_size is not None:
            tmp_member = copy(member)
            tmp_member.array_size = None

            # Generate inner write for each element
            if member.type.name in structs_types:
                tmp_member.name = f"self->{member.name}.data[i]"
                inner_write = _write_type(tmp_member)
            elif member.type.name == "bytes" and member.type.size is not None:
                inner_write = (
                    f"bakelite_write_bytes(buf, self->{member.name}.data[i].data, "
                    f"self->{member.name}.data[i].len)"
                )
            elif member.type.name == "string" and member.type.size is not None:
                inner_write = f"bakelite_write_string(buf, self->{member.name}.data[i])"
            else:
                tmp_member.name = f"self->{member.name}.data[i]"
                inner_write = _write_type(tmp_member)

            return f"""({{ if ((rcode = bakelite_write_uint8(buf, self->{member.name}.len)) != 0) return rcode;
    for (uint8_t i = 0; i < self->{member.name}.len; i++) {{
      if ((rcode = {inner_write}) != 0) return rcode;
    }} 0; }})"""

        # Use self-> prefix if not already present
        name = member.name
        needs_prefix = not (name.startswith("self->") or name.startswith("("))
        if needs_prefix:
            name = f"self->{name}"

        if member.type.name in enums_types:
            underlying = SERIALIZER_SUFFIX_MAP[enums_types[member.type.name].type.name]
            return f"bakelite_write_{underlying}(buf, ({_map_type(enums_types[member.type.name].type)}){name})"

        if member.type.name in structs_types:
            return f"{member.type.name}_pack(&{name}, buf)"

        if member.type.name in SERIALIZER_SUFFIX_MAP:
            suffix = SERIALIZER_SUFFIX_MAP[member.type.name]
            return f"bakelite_write_{suffix}(buf, {name})"

        if member.type.name == "bytes":
            # bytes[N] -> write length prefix + data
            return f"bakelite_write_bytes(buf, {name}.data, {name}.len)"

        if member.type.name == "string":
            # string[N] -> write null-terminated string
            return f"bakelite_write_string(buf, {name})"

        raise RuntimeError(f"Unknown type {member.type.name}")

    def _read_type(member: ProtoStructMember) -> str:
        # Handle arrays (always variable-length now)
        if member.array_size is not None:
            tmp_member = copy(member)
            tmp_member.array_size = None

            # Generate inner read for each element
            if member.type.name in structs_types:
                tmp_member.name = f"self->{member.name}.data[i]"
                inner_read = _read_type(tmp_member)
            elif member.type.name == "bytes" and member.type.size is not None:
                inner_read = (
                    f"bakelite_read_bytes(buf, self->{member.name}.data[i].data, "
                    f"&self->{member.name}.data[i].len, {member.type.size})"
                )
            elif member.type.name == "string" and member.type.size is not None:
                inner_read = (
                    f"bakelite_read_string(buf, self->{member.name}.data[i], "
                    f"{member.type.size + 1})"
                )
            else:
                tmp_member.name = f"self->{member.name}.data[i]"
                inner_read = _read_type(tmp_member)

            return f"""({{
      uint8_t arr_len;
      if ((rcode = bakelite_read_uint8(buf, &arr_len)) != 0) return rcode;
      if (arr_len > {member.array_size}) return BAKELITE_ERR_CAPACITY;
      self->{member.name}.len = arr_len;
      for (uint8_t i = 0; i < arr_len; i++) {{
        if ((rcode = {inner_read}) != 0) return rcode;
      }} 0; }})"""

        # Use self-> prefix if not already present
        name = member.name
        needs_prefix = not (name.startswith("self->") or name.startswith("("))
        if needs_prefix:
            name = f"self->{name}"

        if member.type.name in enums_types:
            underlying = SERIALIZER_SUFFIX_MAP[enums_types[member.type.name].type.name]
            underlying_type = _map_type(enums_types[member.type.name].type)
            enum_type = member.type.name
            return (
                f"({{ {underlying_type} tmp; if ((rcode = bakelite_read_{underlying}(buf, &tmp)) "
                f"!= 0) return rcode; {name} = ({enum_type})tmp; 0; }})"
            )

        if member.type.name in structs_types:
            return f"{member.type.name}_unpack(&{name}, buf)"

        if member.type.name in SERIALIZER_SUFFIX_MAP:
            suffix = SERIALIZER_SUFFIX_MAP[member.type.name]
            return f"bakelite_read_{suffix}(buf, &{name})"

        if member.type.name == "bytes":
            # bytes[N] -> read into inline storage
            if member.type.size is None:
                raise RuntimeError("bytes type must have a size")
            return f"bakelite_read_bytes(buf, {name}.data, &{name}.len, {member.type.size})"

        if member.type.name == "string":
            # string[N] -> read into char array
            if member.type.size is None:
                raise RuntimeError("string type must have a size")
            return f"bakelite_read_string(buf, {name}, {member.type.size + 1})"

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

        # Calculate sizes to determine/validate maxLength
        size_info = calculate_sizes(enums, structs, proto)

        if max_length_opt is None:
            if size_info.max_message_size is None:
                raise RuntimeError(
                    "maxLength must be specified when protocol contains unbounded types"
                )
            max_length = size_info.max_message_size
        else:
            max_length = int(max_length_opt)
            if size_info.max_message_size is not None and max_length < size_info.max_message_size:
                raise RuntimeError(
                    f"maxLength ({max_length}) is less than maximum message size "
                    f"({size_info.max_message_size})"
                )
            if max_length < size_info.min_message_size:
                raise RuntimeError(
                    f"maxLength ({max_length}) is less than minimum message size "
                    f"({size_info.min_message_size})"
                )

        if framing != "cobs":
            raise RuntimeError(f"Unknown framing type {framing}")

    return template.render(
        enums=enums,
        structs=structs,
        proto=proto,
        comments=comments,
        map_type=_map_type,
        map_type_member=_map_type_member,
        inline_struct_def=_inline_struct_def,
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
    runtimes_dir = os.path.join(os.path.dirname(__file__), "runtimes")

    def include(filename: str) -> str:
        # Support both ctiny/ and common/ subdirectories
        if filename.startswith("common/"):
            filepath = os.path.join(runtimes_dir, filename)
        else:
            filepath = os.path.join(runtimes_dir, "ctiny", filename)
        with open(filepath, encoding="utf-8") as f:
            return f.read()

    runtime_template = env.get_template("ctiny-bakelite.h.j2")
    return runtime_template.render(include=include)
