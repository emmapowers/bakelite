"""Tests for protocol functions"""

# pylint: disable=redefined-outer-name,unused-variable,expression-not-assigned,singleton-comparison

import os

import pytest

from bakelite.generator import parse
from bakelite.generator.parser import ValidationError
from bakelite.generator.python import render

FILE_DIR = dir_path = os.path.dirname(os.path.realpath(__file__))


def gen_code(text):
    gbl = globals().copy()

    parsedFile = parse(text)
    generated_code = render(*parsedFile, runtime_import="bakelite.proto")
    exec(generated_code, gbl)
    return gbl


def describe_validation():
    def test_simple_valid(expect):
        gen = gen_code(
            """
      enum TestEnum: uint8 {
        A = 1
        B = 2
      }

      struct TestStruct {
        a: TestEnum
        b: uint16
        c: bytes[64]
      }

      protocol {
        maxLength = 256
        crc = CRC8
        framing = cobs

        messageIds {
          TestStruct = 1
        }
      }
    """
        )

    def test_reserved_message_id(expect):
        code = """
      struct TestStruct {
        a: uint8
      }

      protocol {
        maxLength = 256
        crc = CRC8
        framing = cobs

        messageIds {
          TestStruct = 0
        }
      }
    """
        with pytest.raises(ValidationError) as exinfo:
            gen = gen_code(code)

        expect(str(exinfo.value)).includes("Message ID 0 is reserved for future use")

    def test_missing_message_id_struct(expect):
        code = """
      struct TestStruct {
        a: uint8
      }

      protocol {
        maxLength = 256
        crc = CRC8
        framing = cobs

        messageIds {
          TestStruct = 1
          NotHere = 2
        }
      }
    """
        with pytest.raises(ValidationError) as exinfo:
            gen = gen_code(code)

        expect(str(exinfo.value)).includes("NotHere assigned a message ID, but not declared")
