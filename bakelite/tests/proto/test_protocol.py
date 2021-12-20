"""Tests for protocol functions"""
# pylint: disable=redefined-outer-name,unused-variable,expression-not-assigned,singleton-comparison

import json
import os
from bakelite.proto import parse, List
from bakelite.proto.generator.python import render
from bakelite.proto.runtime import Registry
from bakelite.proto.serialization import struct, SerializationError
from pytest import raises, approx
from dataclasses import dataclass
from bitstring import BitArray, BitStream, ByteStore

FILE_DIR = dir_path = os.path.dirname(os.path.realpath(__file__))

def gen_code(file_name):
    gbl = globals().copy()
        
    with open(file_name) as f:
        text = f.read()
    
    parsedFile = parse(text)
    generated_code = render(*parsedFile)
    exec(generated_code, gbl)
    return gbl

def describe_protocol():
    def test_nested_struct(expect):
        gen = gen_code(FILE_DIR + '/protocol.ex')
        Protocol = gen['Protocol']
        Direction = gen['Direction']
        Speed = gen['Speed']
        Move = gen['Move']
        Ack = gen['Ack']

        buffer = b''

        def read():
            nonlocal buffer
            return buffer
            
        def write(data):
            nonlocal buffer
            buffer=data

        proto = Protocol(
            read=read,
            write=write,
        )

        proto.send(Ack(code=111))
        expect(buffer) == b'\x00\x04\x02o \x00'

        proto2 = Protocol(
            read=read,
            write=write,
        )
        msg = proto2.poll()
        expect(msg) == Ack(code=111)
