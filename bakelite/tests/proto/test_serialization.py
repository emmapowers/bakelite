"""Tests for the frame class, CRC, and data encoding/decoding"""
# pylint: disable=redefined-outer-name,unused-variable,expression-not-assigned,singleton-comparison

from bakelite.proto.serialization import struct, SerializationError
from pytest import raises
from dataclasses import dataclass

def describe_struct():
    def raise_on_non_dataclass(expect):
        with raises(SerializationError):
            @struct
            class Test:
                pass
    
    def dataclass_supported(expect):
        @struct
        @dataclass
        class Test:
            pass

        expect(Test) != None

    def to_bytes(expect):
        @struct
        @dataclass
        class Test:
            pass

        t = Test()

        expect(t.to_bytes()) == b'hello'

    def from_bytes(expect):
        @struct
        @dataclass
        class Test:
            pass

        t = Test()

        expect(t.from_bytes(b'hello')) == None