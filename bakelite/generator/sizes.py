"""Size calculation for protocol types and structs."""

from dataclasses import dataclass
from enum import StrEnum, auto

from .types import PRIMITIVE_TYPES, Protocol, ProtoEnum, ProtoStruct, ProtoStructMember, ProtoType

# Primitive type sizes in bytes
PRIMITIVE_SIZES: dict[str, int] = {
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


class SizeKind(StrEnum):
    """Classification of size characteristics."""

    FIXED = auto()  # Min == Max, no variable components
    BOUNDED = auto()  # Variable but has calculable max (e.g., string[max=32])


@dataclass(frozen=True)
class SizeInfo:
    """Size information for a type or struct."""

    min_size: int
    max_size: int | None  # None means unbounded
    kind: SizeKind

    @property
    def is_fixed(self) -> bool:
        return self.kind == SizeKind.FIXED

    @property
    def is_bounded(self) -> bool:
        return self.kind in (SizeKind.FIXED, SizeKind.BOUNDED)


@dataclass(frozen=True)
class StructSizeInfo:
    """Complete size information for a struct."""

    name: str
    size: SizeInfo


@dataclass(frozen=True)
class ProtocolSizeInfo:
    """Size information for an entire protocol."""

    structs: dict[str, StructSizeInfo]
    message_structs: dict[str, StructSizeInfo]  # Only structs with message IDs

    # Calculated protocol-level info
    min_message_size: int
    max_message_size: int | None  # None if any message is unbounded

    # Framing overhead
    crc_size: int
    framing_overhead: int  # For maxLength buffer calculation

    # RAM requirements for embedded targets
    required_buffer_size: int | None  # None if unbounded
    estimated_ram_ctiny: int | None  # Total RAM for ctiny Protocol struct
    estimated_ram_cpptiny: int | None  # Total RAM for cpptiny ProtocolBase


def cobs_overhead(size: int, crc_size: int) -> int:
    """Calculate COBS overhead for a message size."""
    overhead = int((size + crc_size + 253) / 254)
    return overhead + crc_size + 1  # overhead + CRC + null terminator


class SizeCalculator:
    """Calculate sizes for protocol types."""

    def __init__(
        self,
        enums: list[ProtoEnum],
        structs: list[ProtoStruct],
        protocol: Protocol | None,
    ):
        self.enums = {e.name: e for e in enums}
        self.structs = {s.name: s for s in structs}
        self.protocol = protocol
        self._cache: dict[str, SizeInfo] = {}

    def calc_primitive_size(self, t: ProtoType) -> SizeInfo:
        """Calculate size for a primitive type."""
        if t.name in PRIMITIVE_SIZES:
            size = PRIMITIVE_SIZES[t.name]
            return SizeInfo(size, size, SizeKind.FIXED)

        if t.name == "bytes":
            if t.size is None:
                raise ValueError("bytes type must have a size specifier")
            # bytes[N] - 1-byte length prefix + up to N bytes
            return SizeInfo(1, 1 + t.size, SizeKind.BOUNDED)

        if t.name == "string":
            if t.size is None:
                raise ValueError("string type must have a size specifier")
            # string[N] - null-terminated, up to N chars + null
            return SizeInfo(1, t.size + 1, SizeKind.BOUNDED)

        raise ValueError(f"Unknown primitive type: {t.name}")

    def calc_type_size(self, t: ProtoType) -> SizeInfo:
        """Calculate size for any type (primitive, enum, or struct)."""
        if t.name in PRIMITIVE_TYPES:
            return self.calc_primitive_size(t)

        if t.name in self.enums:
            # Enum uses underlying type size
            enum = self.enums[t.name]
            return self.calc_primitive_size(enum.type)

        if t.name in self.structs:
            return self.calc_struct_size(t.name).size

        raise ValueError(f"Unknown type: {t.name}")

    def calc_member_size(self, member: ProtoStructMember) -> SizeInfo:
        """Calculate size for a struct member (handles arrays)."""
        elem_size = self.calc_type_size(member.type)

        if member.array_size is None:
            # Not an array
            return elem_size

        # All arrays are variable-length: T[N]
        # 1-byte length prefix + 0 to N elements
        min_size = 1  # Just the length byte
        max_size = 1 + member.array_size * elem_size.max_size if elem_size.max_size else None
        kind = SizeKind.BOUNDED if max_size is not None else elem_size.kind
        return SizeInfo(min_size, max_size, kind)

    def calc_struct_size(self, name: str) -> StructSizeInfo:
        """Calculate size for a struct (with caching)."""
        if name in self._cache:
            return StructSizeInfo(name, self._cache[name])

        struct = self.structs[name]

        total_min = 0
        total_max: int | None = 0
        overall_kind = SizeKind.FIXED

        for member in struct.members:
            size = self.calc_member_size(member)

            total_min += size.min_size
            if total_max is not None and size.max_size is not None:
                total_max += size.max_size
            else:
                total_max = None

            # Upgrade kind if needed (BOUNDED takes precedence over FIXED)
            if size.kind == SizeKind.BOUNDED and overall_kind == SizeKind.FIXED:
                overall_kind = SizeKind.BOUNDED

        struct_size = SizeInfo(total_min, total_max, overall_kind)
        self._cache[name] = struct_size

        return StructSizeInfo(name, struct_size)

    def calc_protocol_info(self) -> ProtocolSizeInfo:
        """Calculate complete protocol size information."""
        # Calculate all struct sizes
        struct_infos = {name: self.calc_struct_size(name) for name in self.structs}

        # Get message structs
        message_infos: dict[str, StructSizeInfo] = {}
        if self.protocol:
            for msg_id in self.protocol.message_ids:
                if msg_id.name in struct_infos:
                    message_infos[msg_id.name] = struct_infos[msg_id.name]

        # Calculate message size ranges
        if message_infos:
            min_msg = min(s.size.min_size for s in message_infos.values())
            max_sizes = [s.size.max_size for s in message_infos.values()]
            if all(m is not None for m in max_sizes):
                max_msg: int | None = max(m for m in max_sizes if m is not None)
            else:
                max_msg = None
        else:
            min_msg = 0
            max_msg = 0

        # CRC size
        crc_size = 0
        if self.protocol:
            options = {opt.name: opt.value for opt in self.protocol.options}
            crc = options.get("crc", "none").lower()
            crc_size = {"none": 0, "crc8": 1, "crc16": 2, "crc32": 4}.get(crc, 0)

        # Framing overhead (for COBS)
        overhead = cobs_overhead(max_msg, crc_size) if max_msg is not None else 0

        # Required buffer size
        required_buf = (max_msg + overhead) if max_msg is not None else None

        # RAM estimates for embedded targets (zero-copy: message lives in buffer)
        # ctiny/cpptiny: buffer + framer state (~8 bytes) + protocol state (~16 bytes)
        ram_ctiny: int | None = None
        ram_cpptiny: int | None = None

        if required_buf is not None:
            framer_overhead = 8  # CobsFramer state
            protocol_overhead = 16  # Function pointers and state
            ram_ctiny = required_buf + framer_overhead + protocol_overhead
            ram_cpptiny = required_buf + framer_overhead + protocol_overhead

        return ProtocolSizeInfo(
            structs=struct_infos,
            message_structs=message_infos,
            min_message_size=min_msg,
            max_message_size=max_msg,
            crc_size=crc_size,
            framing_overhead=overhead,
            required_buffer_size=required_buf,
            estimated_ram_ctiny=ram_ctiny,
            estimated_ram_cpptiny=ram_cpptiny,
        )


def calculate_sizes(
    enums: list[ProtoEnum],
    structs: list[ProtoStruct],
    protocol: Protocol | None,
) -> ProtocolSizeInfo:
    """Calculate size information for a protocol definition."""
    calc = SizeCalculator(enums, structs, protocol)
    return calc.calc_protocol_info()
