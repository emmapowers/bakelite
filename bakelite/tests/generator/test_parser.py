"""Tests for protocol parser."""

import pytest

from bakelite.generator import parse
from bakelite.generator.parser import ValidationError
from bakelite.generator.types import ProtoEnum, ProtoStruct


def describe_parse_enum():
    def parses_simple_enum(expect):
        enums, structs, proto, comments = parse(
            """
            enum Color: uint8 {
                Red = 0
                Green = 1
                Blue = 2
            }
        """
        )
        expect(len(enums)) == 1
        expect(enums[0].name) == "Color"
        expect(enums[0].type.name) == "uint8"
        expect(len(enums[0].values)) == 3
        expect(enums[0].values[0].name) == "Red"
        expect(enums[0].values[0].value) == "0"

    def parses_enum_with_different_base_types(expect):
        enums, _, _, _ = parse(
            """
            enum Small: uint8 { A = 0 }
            enum Medium: uint16 { B = 0 }
            enum Large: uint32 { C = 0 }
            enum Signed: int8 { D = 0 }
        """
        )
        expect(len(enums)) == 4
        expect(enums[0].type.name) == "uint8"
        expect(enums[1].type.name) == "uint16"
        expect(enums[2].type.name) == "uint32"
        expect(enums[3].type.name) == "int8"

    def parses_enum_with_comments(expect):
        enums, _, _, comments = parse(
            """
            # Header comment
            enum Status: uint8 {
                OK = 0       # Success
                Error = 1    # Failure
            }
        """
        )
        expect(len(enums)) == 1
        expect(len(comments)) == 1


def describe_parse_struct():
    def parses_simple_struct(expect):
        _, structs, _, _ = parse(
            """
            struct Point {
                x: int32
                y: int32
            }
        """
        )
        expect(len(structs)) == 1
        expect(structs[0].name) == "Point"
        expect(len(structs[0].members)) == 2
        expect(structs[0].members[0].name) == "x"
        expect(structs[0].members[0].type.name) == "int32"

    def parses_all_primitive_types(expect):
        _, structs, _, _ = parse(
            """
            struct AllTypes {
                a: int8
                b: int16
                c: int32
                d: int64
                e: uint8
                f: uint16
                g: uint32
                h: uint64
                i: float32
                j: bool
            }
        """
        )
        expect(len(structs[0].members)) == 10

    def parses_bytes_and_string_types(expect):
        _, structs, _, _ = parse(
            """
            struct Data {
                payload: bytes[64]
                name: string[32]
            }
        """
        )
        expect(structs[0].members[0].type.name) == "bytes"
        expect(structs[0].members[0].type.size) == 64
        expect(structs[0].members[1].type.name) == "string"
        expect(structs[0].members[1].type.size) == 32

    def parses_arrays(expect):
        _, structs, _, _ = parse(
            """
            struct Arrays {
                ints: uint8[5]
                points: Point[10]
            }
            struct Point { x: int32 }
        """
        )
        expect(structs[0].members[0].array_size) == 5
        expect(structs[0].members[1].array_size) == 10

    def parses_nested_struct_reference(expect):
        _, structs, _, _ = parse(
            """
            struct Inner { value: uint8 }
            struct Outer { inner: Inner }
        """
        )
        expect(len(structs)) == 2
        expect(structs[1].members[0].type.name) == "Inner"


def describe_parse_protocol():
    def parses_protocol_block(expect):
        _, _, proto, _ = parse(
            """
            struct Message { data: uint8 }
            protocol {
                maxLength = 256
                crc = CRC8
                framing = cobs
                messageIds {
                    Message = 1
                }
            }
        """
        )
        expect(proto is not None) == True
        expect(len(proto.options)) == 3
        expect(len(proto.message_ids)) == 1
        expect(proto.message_ids[0].name) == "Message"
        expect(proto.message_ids[0].number) == 1

    def parses_without_protocol_block(expect):
        _, structs, proto, _ = parse(
            """
            struct Data { value: uint8 }
        """
        )
        expect(proto is None) == True
        expect(len(structs)) == 1


def describe_parse_annotations():
    def parses_struct_annotations(expect):
        _, structs, _, _ = parse(
            """
            @deprecated
            struct Old { value: uint8 }
        """
        )
        expect(len(structs[0].annotations)) == 1
        expect(structs[0].annotations[0].name) == "deprecated"

    def parses_annotation_with_args(expect):
        _, structs, _, _ = parse(
            """
            @version("1.0")
            struct Versioned { value: uint8 }
        """
        )
        expect(structs[0].annotations[0].name) == "version"
        expect(len(structs[0].annotations[0].arguments)) == 1


def describe_validation():
    def rejects_reserved_message_id_zero(expect):
        with pytest.raises(ValidationError) as exc:
            parse(
                """
                struct Message { data: uint8 }
                protocol {
                    maxLength = 256
                    crc = CRC8
                    framing = cobs
                    messageIds { Message = 0 }
                }
            """
            )
        expect("reserved" in str(exc.value).lower()) == True

    def rejects_undefined_message_id_struct(expect):
        with pytest.raises(ValidationError) as exc:
            parse(
                """
                struct Message { data: uint8 }
                protocol {
                    maxLength = 256
                    crc = CRC8
                    framing = cobs
                    messageIds { Undefined = 1 }
                }
            """
            )
        expect("not declared" in str(exc.value)) == True


def describe_parse_errors():
    def rejects_invalid_syntax(expect):
        with pytest.raises(Exception):
            parse("this is not valid syntax")

    def rejects_unclosed_brace(expect):
        with pytest.raises(Exception):
            parse(
                """
                struct Broken {
                    x: int32
            """
            )
