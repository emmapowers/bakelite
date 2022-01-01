"""Tests for serialization"""
# pylint: disable=redefined-outer-name,unused-variable,expression-not-assigned,singleton-comparison

import json
import os
from dataclasses import dataclass
from io import BytesIO

from pytest import approx, raises

from bakelite.generator import parse
from bakelite.generator.python import render
from bakelite.proto.runtime import Registry
from bakelite.proto.serialization import Packable, SerializationError, struct


FILE_DIR = dir_path = os.path.dirname(os.path.realpath(__file__))

TEST_JSON = json.dumps(
    {'name': 'test', 'members': [], 'comment': '', 'annotations': [], }
)


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
    class Test(Packable):
      pass

    stream = BytesIO()
    t = Test()
    t.pack(stream)

    expect(stream.getvalue()) == b''

  def unpack_empty(expect):
    @struct(Registry(), TEST_JSON)
    @dataclass
    class Test(Packable):
      pass

    stream = BytesIO()
    t = Test()

    expect(Test.unpack(stream)) == Test()

  def test_simple_struct(expect):
    gen = gen_code(FILE_DIR + '/struct.ex')
    Ack = gen['Ack']

    stream = BytesIO()
    ack = Ack(code=123)
    ack.pack(stream)
    expect(stream.getvalue()) == b'\x7b'
    stream.seek(0)
    expect(Ack.unpack(stream)) == Ack(code=123)

  def test_complex_struct(expect):
    gen = gen_code(FILE_DIR + '/struct.ex')
    TestStruct = gen['TestStruct']

    stream = BytesIO()
    test_struct = TestStruct(
        int1=5,
        int2=-1234,
        uint1=31,
        uint2=1234,
        float1=-1.23,
        b1=True,
        b2=True,
        b3=False,
        data=b'\x01\x02\x03\x04',
        str='hey'.encode('ascii'),
    )

    test_struct.pack(stream)
    print(stream.getvalue().hex())
    expect(stream.getvalue()) == bytes.fromhex(
        '052efbffff1fd204a4709dbf010100010203046865790000'
    )
    stream.seek(0)
    new_struct = TestStruct.unpack(stream)

    expect(new_struct) == TestStruct(
        int1=5,
        int2=-1234,
        uint1=31,
        uint2=1234,
        float1=approx(-1.23, 0.001),
        b1=True,
        b2=True,
        b3=False,
        data=b'\x01\x02\x03\x04',
        str='hey'.encode('ascii'),
    )

  def test_enum_struct(expect):
    gen = gen_code(FILE_DIR + '/struct.ex')
    EnumStruct = gen['EnumStruct']
    Direction = gen['Direction']
    Speed = gen['Speed']

    stream = BytesIO()
    test_struct = EnumStruct(direction=Direction.Left, speed=Speed.Fast,)
    test_struct.pack(stream)
    expect(stream.getvalue()) == b'\x02\xff'
    stream.seek(0)
    expect(EnumStruct.unpack(stream)) == EnumStruct(
        direction=Direction.Left, speed=Speed.Fast,
    )

  def test_nested_struct(expect):
    gen = gen_code(FILE_DIR + '/struct.ex')
    NestedStruct = gen['NestedStruct']
    SubA = gen['SubA']
    SubB = gen['SubB']

    stream = BytesIO()
    test_struct = NestedStruct(
        a=SubA(b1=True, b2=False), b=SubB(num=127), num=-4)
    test_struct.pack(stream)
    expect(stream.getvalue()) == b'\x01\x00\x7f\xfc'
    stream.seek(0)
    expect(NestedStruct.unpack(stream)) == NestedStruct(
        a=SubA(b1=True, b2=False), b=SubB(num=127), num=-4
    )

  def test_deeply_nested_struct(expect):
    gen = gen_code(FILE_DIR + '/struct.ex')
    DeeplyNestedStruct = gen['DeeplyNestedStruct']
    SubA = gen['SubA']
    SubC = gen['SubC']

    stream = BytesIO()
    test_struct = DeeplyNestedStruct(c=SubC(a=SubA(b1=False, b2=True)))
    test_struct.pack(stream)
    expect(stream.getvalue()) == b'\x00\x01'
    stream.seek(0)
    expect(DeeplyNestedStruct.unpack(stream)) == DeeplyNestedStruct(
        c=SubC(a=SubA(b1=False, b2=True))
    )

  def test_array_struct(expect):
    gen = gen_code(FILE_DIR + '/struct.ex')
    ArrayStruct = gen['ArrayStruct']
    Direction = gen['Direction']
    Ack = gen['Ack']

    stream = BytesIO()
    test_struct = ArrayStruct(
        a=[Direction.Left, Direction.Right, Direction.Down],
        b=[Ack(code=127), Ack(code=64)],
        c=["abc".encode('ascii'), "def".encode(
            'ascii'), "ghi".encode('ascii')],
    )
    test_struct.pack(stream)
    expect(stream.getvalue()) == bytes.fromhex(
        '0203017f40616263006465660067686900')
    stream.seek(0)
    expect(ArrayStruct.unpack(stream)) == ArrayStruct(
        a=[Direction.Left, Direction.Right, Direction.Down],
        b=[Ack(code=127), Ack(code=64)],
        c=["abc".encode('ascii'), "def".encode(
            'ascii'), "ghi".encode('ascii')],
    )

  def test_variable_types(expect):
    gen = gen_code(FILE_DIR + '/struct.ex')
    VariableLength = gen['VariableLength']

    stream = BytesIO()
    test_struct = VariableLength(
        a=b'hello\x00World',
        b='This is a test string!'.encode('ascii'),
        c=[1, 2, 3, 4],
    )
    test_struct.pack(stream)
    expect(
        stream.getvalue()
    ) == b'\x0bhello\x00WorldThis is a test string!\x00\x04\x01\x02\x03\x04'
    stream.seek(0)
    expect(VariableLength.unpack(stream)) == VariableLength(
        a=b'hello\x00World',
        b='This is a test string!'.encode('ascii'),
        c=[1, 2, 3, 4],
    )
