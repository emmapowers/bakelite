"""Tests for the frame class, CRC, and data encoding/decoding"""
# pylint: disable=redefined-outer-name,unused-variable,expression-not-assigned,singleton-comparison

from pytest import raises

from bakelite.proto import CrcSize, framing


def describe_encoder():
  def encode_zero_length(expect):
    expect(framing.encode(b'')) == b''

  def encode_one_byte(expect):
    expect(framing.encode(b'f')) == b'\x02f'

  def encode_small_string(expect):
    expect(framing.encode(b'hello')) == b'\x06hello'

  def encode_null_byte(expect):
    expect(framing.encode(b'hello\x00world')) == b'\x06hello\x06world'

  def encode_null_terminated(expect):
    expect(framing.encode(b'hello\x00world\x00')
           ) == b'\x06hello\x06world\x01'

  def encode_254_bytes(expect):
    inp = b'A' * 254
    expect(framing.encode(inp)) == b'\xff' + inp

  def encode_500_bytes(expect):
    inpA = b'A' * 254
    inpB = b'A' * 246
    expect(framing.encode(inpA + inpB)) == b'\xff' + inpA + b'\xf7' + inpB

  def decode_zero_length(expect):
    expect(framing.decode(b'')) == b''

  def decode_one_byte(expect):
    expect(framing.decode(b'\02f')) == b'f'

  def decode_null_byte(expect):
    expect(framing.decode(b'\x06hello\x06world')) == b'hello\x00world'

  def decode_null_terminated(expect):
    expect(framing.decode(b'\x06hello\x06world\01')) == b'hello\x00world\00'

  def decode_254_bytes(expect):
    inp = b'A' * 254
    expect(framing.decode(b'\xff' + inp)) == inp

  def decode_500_bytes(expect):
    inpA = b'A' * 254
    inpB = b'A' * 246
    expect(framing.decode(b'\xff' + inpA + b'\xf7' + inpB)) == inpA + inpB

  def decode_unexpected_null(expect):
    with raises(framing.DecodeError):
      framing.decode(b'\x06hello\x00\x06world\01')

  def decode_block_too_long(expect):
    with raises(framing.DecodeError):
      framing.decode(b'\xffhello\x06world')

  def decode_block_too_short(expect):
    with raises(framing.DecodeError):
      framing.decode(b'\x03hello\x06world')

  def encode_decode(expect):
    expect(framing.decode(framing.encode(b'Hello\x00world!'))
           ) == b'Hello\x00world!'


def describe_crc():
  def append_crc_zero_length(expect):
    expect(framing.append_crc(b'')) == b'\x00'

  def append_crc(expect):
    expect(framing.append_crc(b'hello world')) == b'hello world\xa8'

  def check_crc_zero_length(expect):
    with raises(framing.CRCCheckFailure):
      framing.check_crc(b'')

  def check_crc_empty_string(expect):
    expect(framing.check_crc(b'\x00')) == b''

  def check_crc(expect):
    expect(framing.check_crc(b'hello world\xa8')) == b'hello world'

  def check_crc_failure(expect):
    with raises(framing.CRCCheckFailure):
      framing.check_crc(b'hello world\xff')

  def check_crc_missing_crc(expect):
    with raises(framing.CRCCheckFailure):
      framing.check_crc(b'hello world')

  def append_crc_8bit(expect):
    expect(
        framing.append_crc(
            b'hello world', crc_size=CrcSize.CRC8)
    ) == b'hello world\xa8'

  def check_crc_8bit(expect):
    expect(
        framing.check_crc(b'hello world\xa8', crc_size=CrcSize.CRC8)
    ) == b'hello world'

  def append_crc_16bit(expect):
    expect(
        framing.append_crc(
            b'hello world', crc_size=CrcSize.CRC16)
    ) == b'hello world\xc19'

  def check_crc_16bit(expect):
    expect(
        framing.check_crc(b'hello world\xc19', crc_size=CrcSize.CRC16)
    ) == b'hello world'

  def append_crc_32bit(expect):
    expect(
        framing.append_crc(
            b'hello world', crc_size=CrcSize.CRC32)
    ) == b'hello world\x85\x11J\r'

  def check_crc_32bit(expect):
    expect(
        framing.check_crc(b'hello world\x85\x11J\r', crc_size=CrcSize.CRC32)
    ) == b'hello world'


def describe_framer():
  def encode_frame(expect):
    framer = framing.Framer()
    expect(
        framer.encode_frame(b'hello\x00world')
    ) == b'\x00\x06hello\x07world\x93\x00'

  def encode_frame_no_crc(expect):
    framer = framing.Framer(crc=CrcSize.NO_CRC)
    expect(framer.encode_frame(b'hello\x00world')
           ) == b'\x00\x06hello\x06world\x00'

  def encode_frame_crc_32(expect):
    framer = framing.Framer(crc=CrcSize.CRC32)
    expect(framer.encode_frame(b'hello\x00world')
           ) == b'\x00\x06hello\nworld\xb3\x14\xe6\n\x00'

  def encode_frame_zero_lenth(expect):
    framer = framing.Framer()
    with raises(framing.EncodeError):
      framer.encode_frame(b'')

  def decode_frame(expect):
    framer = framing.Framer()
    framer.append_buffer(b'\x06hello\x07world\x93\x00')
    expect(framer.decode_frame()) == b'hello\x00world'

  def decode_frame_crc_32(expect):
    framer = framing.Framer(crc=CrcSize.CRC32)
    framer.append_buffer(b'\x00\x06hello\nworld\xb3\x14\xe6\n\x00')
    expect(framer.decode_frame()) == b'hello\x00world'

  def decode_frame_no_crc(expect):
    framer = framing.Framer(crc=CrcSize.NO_CRC)
    framer.append_buffer(b'\x06hello\x06world\x00')
    expect(framer.decode_frame()) == b'hello\x00world'

  def decode_frame_preceeding_null(expect):
    framer = framing.Framer()
    framer.append_buffer(b'\x00\x06hello\x07world\x93\x00')
    expect(framer.decode_frame()) == b'hello\x00world'

  def decode_frame_crc_failure(expect):
    framer = framing.Framer()
    framer.append_buffer(b'\x06hello\x07wOr!d\x93\x00')
    with raises(framing.CRCCheckFailure):
      framer.decode_frame()

  def decode_frame_crc_recovery(expect):
    framer = framing.Framer()
    framer.append_buffer(b'\x06hello\x07wOr!d\x93\x00')
    framer.append_buffer(b'\x06hello\x07world\x93\x00')

    with raises(framing.CRCCheckFailure):
      framer.decode_frame()

    expect(framer.decode_frame()) == b'hello\x00world'

  def decode_frame_no_data(expect):
    framer = framing.Framer()
    expect(framer.decode_frame()) == None

  def decode_partial_frame(expect):
    framer = framing.Framer()
    framer.append_buffer(b'\x06hello\x07wo')
    expect(framer.decode_frame()) == None

    framer.append_buffer(b'rld\x93\x00')
    expect(framer.decode_frame()) == b'hello\x00world'

  def decode_clear_buffer(expect):
    framer = framing.Framer()
    framer.append_buffer(b'\x00\x06hello\x07world\x93\x00')
    framer.clear_buffer()
    expect(framer.decode_frame()) == None
