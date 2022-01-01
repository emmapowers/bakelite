"""Tests for protocol functions"""
# pylint: disable=redefined-outer-name,unused-variable,expression-not-assigned,singleton-comparison

import json
import os
from io import BytesIO

from bakelite.generator import parse
from bakelite.generator.python import render


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

    stream = BytesIO()

    proto = Protocol(stream=stream)

    proto.send(Ack(code=111))
    expect(stream.getvalue()) == b'\x00\x04\x02o \x00'

    stream.seek(0)
    proto2 = Protocol(stream=stream)
    msg = proto2.poll()
    expect(msg) == Ack(code=111)
