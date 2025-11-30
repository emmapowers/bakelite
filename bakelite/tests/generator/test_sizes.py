"""Tests for size calculation."""

from bakelite.generator import parse
from bakelite.generator.sizes import SizeKind, calculate_sizes


def describe_primitive_sizes():
    def calculates_fixed_primitives(expect):
        proto = """
        struct Fixed {
            a: uint8
            b: int32
            c: float64
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.structs["Fixed"].size.min_size) == 13  # 1 + 4 + 8
        expect(info.structs["Fixed"].size.max_size) == 13
        expect(info.structs["Fixed"].size.kind) == SizeKind.FIXED

    def calculates_bool_size(expect):
        proto = """
        struct Bools {
            a: bool
            b: bool
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.structs["Bools"].size.min_size) == 2
        expect(info.structs["Bools"].size.max_size) == 2


def describe_bytes_and_string_sizes():
    def calculates_bytes_size(expect):
        proto = """
        struct BytesField {
            data: bytes[64]
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        # bytes[N] - 1-byte length prefix + up to N bytes
        expect(info.structs["BytesField"].size.min_size) == 1  # length byte only
        expect(info.structs["BytesField"].size.max_size) == 65  # 1 + 64
        expect(info.structs["BytesField"].size.kind) == SizeKind.BOUNDED

    def calculates_string_size(expect):
        proto = """
        struct StringField {
            text: string[64]
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        # string[N] - null-terminated, up to N chars + null
        expect(info.structs["StringField"].size.min_size) == 1  # null terminator
        expect(info.structs["StringField"].size.max_size) == 65  # up to 64 chars + null
        expect(info.structs["StringField"].size.kind) == SizeKind.BOUNDED


def describe_array_sizes():
    def calculates_array_size(expect):
        proto = """
        struct ArrayField {
            values: uint8[16]
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        # All arrays are variable-length: 1-byte length prefix + up to N elements
        expect(info.structs["ArrayField"].size.min_size) == 1  # length byte only
        expect(info.structs["ArrayField"].size.max_size) == 17  # 1 + 16*1
        expect(info.structs["ArrayField"].size.kind) == SizeKind.BOUNDED

    def calculates_array_of_larger_type(expect):
        proto = """
        struct Array32 {
            values: int32[16]
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.structs["Array32"].size.min_size) == 1
        expect(info.structs["Array32"].size.max_size) == 1 + 16 * 4  # 65
        expect(info.structs["Array32"].size.kind) == SizeKind.BOUNDED


def describe_nested_structs():
    def calculates_nested_struct_size(expect):
        proto = """
        struct Inner {
            x: uint8
        }
        struct Outer {
            inner: Inner
            y: uint16
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.structs["Inner"].size.min_size) == 1
        expect(info.structs["Outer"].size.min_size) == 3  # 1 + 2
        expect(info.structs["Outer"].size.max_size) == 3

    def calculates_deeply_nested_size(expect):
        proto = """
        struct A {
            x: uint8
        }
        struct B {
            a: A
            y: uint8
        }
        struct C {
            b: B
            z: uint8
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.structs["C"].size.min_size) == 3
        expect(info.structs["C"].size.max_size) == 3


def describe_enums():
    def calculates_enum_size(expect):
        proto = """
        enum Status: uint16 {
            OK = 0
            ERROR = 1
        }
        struct WithEnum {
            status: Status
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.structs["WithEnum"].size.min_size) == 2
        expect(info.structs["WithEnum"].size.max_size) == 2

    def calculates_enum_array_size(expect):
        proto = """
        enum Direction: uint8 {
            Up = 0
            Down = 1
        }
        struct WithEnumArray {
            dirs: Direction[4]
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        # All arrays are variable-length: 1-byte length prefix + up to 4 enum values
        expect(info.structs["WithEnumArray"].size.min_size) == 1  # length byte only
        expect(info.structs["WithEnumArray"].size.max_size) == 5  # 1 + 4
        expect(info.structs["WithEnumArray"].size.kind) == SizeKind.BOUNDED


def describe_protocol_info():
    def calculates_message_sizes(expect):
        proto = """
        struct Msg1 {
            a: uint8
        }
        struct Msg2 {
            b: uint32
        }
        struct Helper {
            x: uint8
        }
        protocol {
            maxLength = 100
            framing = COBS
            crc = CRC8
            messageIds {
                Msg1 = 1
                Msg2 = 2
            }
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.min_message_size) == 1  # Msg1
        expect(info.max_message_size) == 4  # Msg2
        expect(info.crc_size) == 1
        expect(len(info.message_structs)) == 2
        expect("Helper" not in info.message_structs) == True

    def calculates_framing_overhead(expect):
        proto = """
        struct Msg {
            data: bytes[64]
        }
        protocol {
            maxLength = 100
            framing = COBS
            crc = CRC32
            messageIds {
                Msg = 1
            }
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        # bytes[64] max size is 65 (1 len + 64 data)
        # COBS overhead: int((65+4+253)/254) + 4 + 1 = 1 + 4 + 1 = 6
        expect(info.crc_size) == 4
        expect(info.framing_overhead) == 6
        expect(info.required_buffer_size) == 71  # 65 + 6

    def handles_no_crc(expect):
        proto = """
        struct Msg {
            a: uint8
        }
        protocol {
            maxLength = 10
            framing = COBS
            crc = None
            messageIds {
                Msg = 1
            }
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.crc_size) == 0

    def handles_bounded_variable_messages(expect):
        proto = """
        struct Bounded {
            text: string[64]
        }
        protocol {
            maxLength = 100
            framing = COBS
            crc = CRC8
            messageIds {
                Bounded = 1
            }
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.max_message_size) == 65  # 64 + 1 null
        expect(info.required_buffer_size) is not None
        expect(info.framing_overhead) > 0


def describe_mixed_structs():
    def propagates_bounded_string_through_struct(expect):
        proto = """
        struct Inner {
            text: string[64]
        }
        struct Outer {
            a: uint8
            inner: Inner
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.structs["Outer"].size.kind) == SizeKind.BOUNDED
        expect(info.structs["Outer"].size.max_size) == 66  # 1 + 65 (max string + null)
        expect(info.structs["Outer"].size.min_size) == 2  # 1 + 1 (null)

    def propagates_bounded_bytes_through_struct(expect):
        proto = """
        struct Inner {
            data: bytes[64]
        }
        struct Outer {
            a: uint8
            inner: Inner
        }
        """
        enums, structs, protocol, _ = parse(proto)
        info = calculate_sizes(enums, structs, protocol)

        expect(info.structs["Outer"].size.kind) == SizeKind.BOUNDED
        expect(info.structs["Outer"].size.min_size) == 2  # 1 + 1
        expect(info.structs["Outer"].size.max_size) == 66  # 1 + 65 (1 len + 64 data)
