"""Python code generator for bakelite protocols."""

from importlib import resources

from jinja2 import Environment, PackageLoader

from .types import Protocol, ProtoEnum, ProtoStruct, ProtoStructMember, ProtoType
from .util import to_camel_case

RUNTIME_FILES = [
    "__init__.py",
    "types.py",
    "serialization.py",
    "runtime.py",
    "framing.py",
    "crc.py",
]

env = Environment(
    loader=PackageLoader("bakelite.generator", "templates"),
    trim_blocks=True,
    lstrip_blocks=True,
    keep_trailing_newline=True,
    line_comment_prefix="%%",
    line_statement_prefix="%",
)

template = env.get_template("python.py.j2")

# Map bakelite types to Python type annotations
PRIMITIVE_TYPE_MAP = {
    "bool": "bool",
    "int8": "int",
    "int16": "int",
    "int32": "int",
    "int64": "int",
    "uint8": "int",
    "uint16": "int",
    "uint32": "int",
    "uint64": "int",
    "float32": "float",
    "float64": "float",
    "bytes": "bytes",
    "string": "str",
}

# Map bakelite types to struct format characters
FORMAT_CHARS = {
    "bool": "?",
    "int8": "b",
    "uint8": "B",
    "int16": "h",
    "uint16": "H",
    "int32": "i",
    "uint32": "I",
    "int64": "q",
    "uint64": "Q",
    "float32": "f",
    "float64": "d",
}

# Size in bytes for each type
TYPE_SIZES = {
    "bool": 1,
    "int8": 1,
    "uint8": 1,
    "int16": 2,
    "uint16": 2,
    "int32": 4,
    "uint32": 4,
    "int64": 8,
    "uint64": 8,
    "float32": 4,
    "float64": 8,
}

PRIMITIVES = frozenset(PRIMITIVE_TYPE_MAP.keys())


def _map_type(member: ProtoStructMember) -> str:
    """Map a proto type to a Python type annotation."""
    type_name = PRIMITIVE_TYPE_MAP.get(member.type.name, member.type.name)

    if member.array_size is not None:
        return f"list[{type_name}]"
    return type_name


def _is_primitive(t: ProtoType) -> bool:
    """Check if a type is a primitive type."""
    return t.name in PRIMITIVES


def _is_numeric_primitive(t: ProtoType) -> bool:
    """Check if a type is a numeric primitive (can use struct.pack)."""
    return t.name in FORMAT_CHARS


def _format_char(t: ProtoType) -> str:
    """Get the struct format character for a type."""
    return FORMAT_CHARS.get(t.name, "")


def _type_size(t: ProtoType) -> int:
    """Get the size in bytes of a type."""
    return TYPE_SIZES.get(t.name, 0)


def _gen_pack_primitive(member: ProtoStructMember, indent: str = "") -> str:
    """Generate pack code for a primitive field."""
    t = member.type
    name = member.name

    if t.name in FORMAT_CHARS:
        fmt = FORMAT_CHARS[t.name]
        return f'{indent}_buf.extend(_struct.pack("={fmt}", self.{name}))'

    if t.name == "bytes":
        if t.size is None or t.size == 0:
            # Variable length bytes
            return (
                f"{indent}if len(self.{name}) > 255:\n"
                f'{indent}    raise SerializationError("{name} exceeds 255 bytes")\n'
                f'{indent}_buf.extend(_struct.pack("=B", len(self.{name})))\n'
                f"{indent}_buf.extend(self.{name})"
            )
        # Fixed length bytes
        return (
            f"{indent}if len(self.{name}) > {t.size}:\n"
            f'{indent}    raise SerializationError("{name} exceeds {t.size} bytes")\n'
            f"{indent}_buf.extend(self.{name})\n"
            f'{indent}_buf.extend(b"\\x00" * ({t.size} - len(self.{name})))'
        )

    if t.name == "string":
        if t.size is None or t.size == 0:
            # Variable length string (null terminated)
            return (
                f'{indent}_enc_{name} = self.{name}.encode("ascii")\n'
                f"{indent}_buf.extend(_enc_{name})\n"
                f"{indent}_buf.append(0)"
            )
        # Fixed length string
        return (
            f'{indent}_enc_{name} = self.{name}.encode("ascii")\n'
            f"{indent}if len(_enc_{name}) >= {t.size}:\n"
            f'{indent}    raise SerializationError("{name} exceeds {t.size - 1} chars")\n'
            f"{indent}_buf.extend(_enc_{name})\n"
            f'{indent}_buf.extend(b"\\x00" * ({t.size} - len(_enc_{name})))'
        )

    raise ValueError(f"Unknown primitive type: {t.name}")


def _gen_unpack_primitive(member: ProtoStructMember, indent: str = "") -> str:
    """Generate unpack code for a primitive field."""
    t = member.type
    name = member.name

    if t.name in FORMAT_CHARS:
        fmt = FORMAT_CHARS[t.name]
        size = TYPE_SIZES[t.name]
        return (
            f'{indent}{name} = _struct.unpack_from("={fmt}", _data, _o)[0]\n'
            f"{indent}_o += {size}"
        )

    if t.name == "bytes":
        if t.size is None or t.size == 0:
            # Variable length bytes
            return (
                f'{indent}_len_{name} = _struct.unpack_from("=B", _data, _o)[0]\n'
                f"{indent}_o += 1\n"
                f"{indent}{name} = bytes(_data[_o:_o + _len_{name}])\n"
                f"{indent}_o += _len_{name}"
            )
        # Fixed length bytes
        return f"{indent}{name} = bytes(_data[_o:_o + {t.size}])\n" f"{indent}_o += {t.size}"

    if t.name == "string":
        if t.size is None or t.size == 0:
            # Variable length string (null terminated)
            return (
                f'{indent}_end_{name} = _data.find(b"\\x00", _o)\n'
                f"{indent}if _end_{name} < 0:\n"
                f'{indent}    raise SerializationError("unterminated string {name}")\n'
                f'{indent}{name} = bytes(_data[_o:_end_{name}]).decode("ascii")\n'
                f"{indent}_o = _end_{name} + 1"
            )
        # Fixed length string
        return (
            f"{indent}_raw_{name} = _data[_o:_o + {t.size}]\n"
            f'{indent}_null_{name} = _raw_{name}.find(b"\\x00")\n'
            f'{indent}{name} = bytes(_raw_{name}[:_null_{name} if _null_{name} >= 0 else {t.size}]).decode("ascii")\n'
            f"{indent}_o += {t.size}"
        )

    raise ValueError(f"Unknown primitive type: {t.name}")


def _gen_pack_field(
    member: ProtoStructMember, enums: list[ProtoEnum], structs: list[ProtoStruct]
) -> str:
    """Generate pack code for a struct field."""
    t = member.type
    name = member.name
    lines: list[str] = []

    # Check if it's an array
    if member.array_size is not None:
        if member.array_size == 0:
            # Variable length array - write length prefix
            lines.append(f"if len(self.{name}) > 255:")
            lines.append(f'    raise SerializationError("{name} array exceeds 255 elements")')
            lines.append(f'_buf.extend(_struct.pack("=B", len(self.{name})))')

        # Check if it's a primitive array that can be batched
        if t.name in FORMAT_CHARS:
            fmt = FORMAT_CHARS[t.name]
            if member.array_size == 0:
                # Variable length - use f-string format
                lines.append(
                    f'_buf.extend(_struct.pack(f"={{len(self.{name})}}{fmt}", *self.{name}))'
                )
            else:
                # Fixed length - use literal format
                lines.append(f"if len(self.{name}) != {member.array_size}:")
                lines.append(
                    f'    raise SerializationError("{name} must have {member.array_size} elements")'
                )
                lines.append(
                    f'_buf.extend(_struct.pack("={member.array_size}{fmt}", *self.{name}))'
                )
        else:
            # Non-primitive array - loop
            lines.append(f"for _item in self.{name}:")
            if _is_primitive(t):
                inner = _gen_pack_primitive(
                    ProtoStructMember(
                        type=t,
                        name="_item",
                        value=None,
                        comment=None,
                        annotations=[],
                        array_size=None,
                    ),
                    indent="",
                )
                # Replace self._item with _item
                inner = inner.replace("self._item", "_item")
                # Indent each line properly
                for line in inner.split("\n"):
                    lines.append("    " + line)
            elif _is_enum(t, enums):
                lines.append("    _buf.extend(_item.pack())")
            else:
                lines.append("    _buf.extend(_item.pack())")
        return "\n".join(lines)

    # Single value
    if _is_primitive(t):
        return _gen_pack_primitive(member)
    if _is_enum(t, enums):
        return f"_buf.extend(self.{name}.pack())"
    # Nested struct
    return f"_buf.extend(self.{name}.pack())"


def _gen_unpack_field(
    member: ProtoStructMember, enums: list[ProtoEnum], structs: list[ProtoStruct]
) -> str:
    """Generate unpack code for a struct field."""
    t = member.type
    name = member.name
    lines: list[str] = []

    # Check if it's an array
    if member.array_size is not None:
        if member.array_size == 0:
            # Variable length array - read length prefix
            lines.append(f'_len_{name} = _struct.unpack_from("=B", _data, _o)[0]')
            lines.append("_o += 1")

        # Check if it's a primitive array that can be batched
        if t.name in FORMAT_CHARS:
            fmt = FORMAT_CHARS[t.name]
            size = TYPE_SIZES[t.name]
            if member.array_size == 0:
                # Variable length
                lines.append(
                    f'{name} = list(_struct.unpack_from(f"={{_len_{name}}}{fmt}", _data, _o))'
                )
                lines.append(f"_o += _len_{name} * {size}")
            else:
                # Fixed length
                lines.append(
                    f'{name} = list(_struct.unpack_from("={member.array_size}{fmt}", _data, _o))'
                )
                lines.append(f"_o += {member.array_size * size}")
        else:
            # Non-primitive array - loop
            arr_len = f"_len_{name}" if member.array_size == 0 else str(member.array_size)
            lines.append(f"{name} = []")
            lines.append(f"for _ in range({arr_len}):")
            if _is_primitive(t):
                # For primitive non-numeric types (bytes/string in arrays is unusual)
                inner = _gen_unpack_primitive(
                    ProtoStructMember(
                        type=t,
                        name="_item",
                        value=None,
                        comment=None,
                        annotations=[],
                        array_size=None,
                    ),
                    indent="",
                )
                # Indent each line properly
                for line in inner.split("\n"):
                    lines.append("    " + line)
                lines.append(f"    {name}.append(_item)")
            elif _is_enum(t, enums):
                lines.append(f"    _item, _n = {t.name}.unpack(_data, _o)")
                lines.append("    _o += _n")
                lines.append(f"    {name}.append(_item)")
            else:
                lines.append(f"    _item, _n = {t.name}.unpack(_data, _o)")
                lines.append("    _o += _n")
                lines.append(f"    {name}.append(_item)")
        return "\n".join(lines)

    # Single value
    if _is_primitive(t):
        return _gen_unpack_primitive(member)
    if _is_enum(t, enums):
        return f"{name}, _n = {t.name}.unpack(_data, _o)\n_o += _n"
    # Nested struct
    return f"{name}, _n = {t.name}.unpack(_data, _o)\n_o += _n"


def _is_enum(t: ProtoType, enums: list[ProtoEnum]) -> bool:
    """Check if a type is an enum."""
    return any(e.name == t.name for e in enums)


def _is_struct(t: ProtoType, structs: list[ProtoStruct]) -> bool:
    """Check if a type is a struct."""
    return any(s.name == t.name for s in structs)


def _can_batch(member: ProtoStructMember, enums: list[ProtoEnum]) -> bool:
    """Check if member can be batched with other primitives."""
    if member.array_size is not None:
        return False
    if member.type.name in ("bytes", "string"):
        return False
    if _is_enum(member.type, enums):
        return False
    return member.type.name in FORMAT_CHARS


def _batch_members(
    members: list[ProtoStructMember], enums: list[ProtoEnum]
) -> list[tuple[str, list[ProtoStructMember]]]:
    """Group members into batches for pack/unpack optimization.

    Returns list of (batch_type, members) where batch_type is "primitive" or "single".
    """
    batches: list[tuple[str, list[ProtoStructMember]]] = []
    current: list[ProtoStructMember] = []

    for member in members:
        if _can_batch(member, enums):
            current.append(member)
        else:
            if current:
                batches.append(("primitive", current))
                current = []
            batches.append(("single", [member]))

    if current:
        batches.append(("primitive", current))

    return batches


def _gen_pack_batch(members: list[ProtoStructMember]) -> str:
    """Generate pack code for a batch of primitives."""
    fmt = "=" + "".join(FORMAT_CHARS[m.type.name] for m in members)
    args = ", ".join(f"self.{m.name}" for m in members)
    return f'_buf.extend(_struct.pack("{fmt}", {args}))'


def _gen_unpack_batch(members: list[ProtoStructMember]) -> str:
    """Generate unpack code for a batch of primitives."""
    fmt = "=" + "".join(FORMAT_CHARS[m.type.name] for m in members)
    size = sum(TYPE_SIZES[m.type.name] for m in members)
    names = ", ".join(m.name for m in members)
    # Add trailing comma for single values so tuple unpacking works: val, = (1,)
    if len(members) == 1:
        names += ","
    return f'{names} = _struct.unpack_from("{fmt}", _data, _o)\n_o += {size}'


def _field_args(member: ProtoStructMember) -> str:
    """Generate additional arguments for bakelite_field()."""
    args: list[str] = []
    if member.type.size is not None and member.type.size > 0:
        args.append(f"max_length={member.type.size}")
    if member.array_size is not None:
        args.append(f"array_size={member.array_size}")
    if args:
        return ", " + ", ".join(args)
    return ""


def render(
    enums: list[ProtoEnum],
    structs: list[ProtoStruct],
    proto: Protocol | None,
    comments: list[str],
    runtime_import: str = "bakelite_runtime",
) -> str:
    """Render a protocol definition to Python source code."""
    return template.render(
        enums=enums,
        structs=structs,
        proto=proto,
        comments=comments,
        map_type=_map_type,
        is_primitive=_is_primitive,
        format_char=_format_char,
        type_size=_type_size,
        gen_pack_field=lambda m: _gen_pack_field(m, enums, structs),
        gen_unpack_field=lambda m: _gen_unpack_field(m, enums, structs),
        batch_members=lambda members: _batch_members(members, enums),
        gen_pack_batch=_gen_pack_batch,
        gen_unpack_batch=_gen_unpack_batch,
        field_args=_field_args,
        to_camel_case=to_camel_case,
        runtime_import=runtime_import,
        BLANK_LINE="",
    )


def runtime() -> dict[str, str]:
    """Return the Python runtime files as a dict of filename -> content."""
    result: dict[str, str] = {}
    for filename in RUNTIME_FILES:
        content = resources.files("bakelite.proto").joinpath(filename).read_text()
        result[filename] = content
    return result
