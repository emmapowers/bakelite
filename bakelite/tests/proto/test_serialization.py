"""Tests for serialization"""
# pylint: disable=redefined-outer-name,unused-variable,expression-not-assigned,singleton-comparison

import json
import os
from bakelite.proto import parse, List
from bakelite.proto.generator.python import render
from bakelite.proto.runtime import Registry
from bakelite.proto.serialization import struct, SerializationError
from pytest import raises, approx
from dataclasses import dataclass
from bitstring import BitArray, BitStream

FILE_DIR = dir_path = os.path.dirname(os.path.realpath(__file__))

TEST_JSON=json.dumps({
    'name': 'test',
    'members': [],
    'comment': '',
    'annotations': [],
})

def gen_code(file_name):
    gbl = globals().copy()
        
    with open(file_name) as f:
        text = f.read()
    
    parsedFile = parse(text)
    generated_code = render(*parsedFile)
    exec(generated_code, gbl)
    return gbl

def describe_serialization():
    def raise_on_non_dataclass(expect):
        with raises(SerializationError):
            @struct(Registry(), TEST_JSON)
            class Test:
                pass
    
    def dataclass_supported(expect):
        @struct(Registry(), TEST_JSON)
        @dataclass
        class Test:
            pass

        expect(Test) != None

    def pack_empty(expect):
        @struct(Registry(), TEST_JSON)
        @dataclass
        class Test:
            pass

        t = Test()

        expect(t.pack()) == b''

    def unpack_empty(expect):
        @struct(Registry(), TEST_JSON)
        @dataclass
        class Test:
            pass

        t = Test()

        expect(Test.unpack(b'hello')) == Test()
    
    def test_simple_struct(expect):
        gen = gen_code(FILE_DIR + '/struct.ex')
        Ack = gen['Ack']

        ack = Ack(code=123)
        data = ack.pack()
        expect(data) == BitStream('0x7b')
        expect(Ack.unpack(data)) == Ack(code=123)

    def test_complex_struct(expect):
        gen = gen_code(FILE_DIR + '/struct.ex')
        TestStruct = gen['TestStruct']

        test_struct = TestStruct(
            int1=5,
            int2=-1234,
            uint1=31,
            uint2=1234,
            float1=-1.23,
            f1=True,
            f2=True,
            f3=False,
            b1=[1, 0, 1, 0, 1],
            b2=b'\x01\x02\x03\x04',
            s1='hey'.encode('ascii'),
        )
        data = test_struct.pack()
        expect(data) == BitStream('0x5fffffb2ef82695fceb8526a808101823432bc8000, 0b0')
        new_struct = TestStruct.unpack(data)

        # test the float field seperatly
        expect(new_struct.float1) == approx(-1.23, 0.001)
        new_struct.float1 = 0

        expect(new_struct) == TestStruct(
            int1=5,
            int2=-1234,
            uint1=31,
            uint2=1234,
            float1=-0,
            f1=True,
            f2=True,
            f3=False,
            b1=[1, 0, 1, 0, 1],
            b2=b'\x01\x02\x03\x04',
            s1='hey'.encode('ascii'),
        )
    
    def test_enum_struct(expect):
        gen = gen_code(FILE_DIR + '/struct.ex')
        EnumStruct = gen['EnumStruct']
        Direction = gen['Direction']
        Speed = gen['Speed']

        test_struct = EnumStruct(
            direction=Direction.Left,
            speed=Speed.Fast,
        )
        data = test_struct.pack()
        expect(data) == BitStream('0b1011111111')
        expect(EnumStruct.unpack(data)) == EnumStruct(
            direction=Direction.Left,
            speed=Speed.Fast,
        )

    def test_nested_struct(expect):
        gen = gen_code(FILE_DIR + '/struct.ex')
        NestedStruct = gen['NestedStruct']
        SubA = gen['SubA']
        SubB = gen['SubB']

        test_struct = NestedStruct(
            a=SubA(flag=True, flag2=False),
            b=SubB(num=127),
            num=-4
        )
        data = test_struct.pack()
        expect(data) == BitStream('0b10011111111100')
        expect(NestedStruct.unpack(data)) == NestedStruct(
            a=SubA(flag=True, flag2=False),
            b=SubB(num=127),
            num=-4
        )

    def test_deeply_nested_struct(expect):
        gen = gen_code(FILE_DIR + '/struct.ex')
        DeeplyNestedStruct = gen['DeeplyNestedStruct']
        SubA = gen['SubA']
        SubC = gen['SubC']

        test_struct = DeeplyNestedStruct(
            c=SubC(a=SubA(flag=False, flag2=True))
        )
        data = test_struct.pack()
        expect(data) == BitStream('0b01')
        expect(DeeplyNestedStruct.unpack(data)) == DeeplyNestedStruct(
            c=SubC(a=SubA(flag=False, flag2=True))
        )

    def test_nested_struct(expect):
        gen = gen_code(FILE_DIR + '/struct.ex')
        ArrayStruct = gen['ArrayStruct']
        Direction = gen['Direction']
        Ack = gen['Ack']

        test_struct = ArrayStruct(
            a=[Direction.Left, Direction.Right, Direction.Down],
            b=[Ack(code=127), Ack(code=64)]
        )
        data = test_struct.pack()
        expect(data) == BitStream('0b1011010111111101000000')
        expect(ArrayStruct.unpack(data)) == ArrayStruct(
            a=[Direction.Left, Direction.Right, Direction.Down],
            b=[Ack(code=127), Ack(code=64)]
        )
