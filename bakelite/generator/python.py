"""Python code generator for bakelite protocols."""

from importlib import resources

from jinja2 import Environment, PackageLoader

from .types import Protocol, ProtoEnum, ProtoStruct, ProtoStructMember
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


def _map_type(member: ProtoStructMember) -> str:
    """Map a proto type to a Python type annotation."""
    type_name = PRIMITIVE_TYPE_MAP.get(member.type.name, member.type.name)

    if member.array_size is not None:
        return f"list[{type_name}]"
    return type_name


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
