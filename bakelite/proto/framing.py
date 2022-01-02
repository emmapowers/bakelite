from enum import Enum
from typing import Optional

import crcmod


class CrcSize(Enum):
  NO_CRC = 0
  CRC8 = 1
  CRC16 = 2
  CRC32 = 4


g_crc_funcs = {
    CrcSize.CRC8: crcmod.predefined.mkPredefinedCrcFun('crc-8'),
    CrcSize.CRC16: crcmod.predefined.mkPredefinedCrcFun('crc-16'),
    CrcSize.CRC32: crcmod.predefined.mkPredefinedCrcFun('crc-32'),
}


class FrameError(RuntimeError):
  pass


class EncodeError(FrameError):
  pass


class DecodeError(FrameError):
  pass


class CRCCheckFailure(FrameError):
  pass


def _append_block(block: bytearray, output: bytearray):
  output.append(len(block) + 1)
  output.extend(block)
  block.clear()


def encode(data: bytes) -> bytes:
  block = bytearray()
  output = bytearray()
  full_block = False

  if not data:
    return b''

  for byte in data:
    full_block = False
    if byte == 0:
      _append_block(block, output)
    else:
      block.append(byte)
      if len(block) == 254:
        _append_block(block, output)
        full_block = True

  if not full_block:
    _append_block(block, output)

  return output


def decode(data: bytes):
  output = bytearray()

  if not data:
    return b''

  while len(data) > 0:
    block_size = data[0]

    if block_size == 0:
      raise DecodeError("Unexpected null byte")

    if block_size > len(data):
      raise DecodeError("Block length exceeds size of available data")

    block = data[1:block_size]
    data = data[block_size:]

    output.extend(block)
    if block_size != 255:
      output.append(0)

  if output[-1] == 0:
    output.pop()

  return output


def append_crc(data: bytes, crc_size=CrcSize.CRC8):
  return data + g_crc_funcs[crc_size](data).to_bytes(crc_size.value, byteorder='little')


def check_crc(data: bytes, crc_size=CrcSize.CRC8):
  if not data:
    raise CRCCheckFailure()

  crc_val = int.from_bytes(data[-crc_size.value:], byteorder='little')
  output = data[:-crc_size.value]

  if g_crc_funcs[crc_size](output) != crc_val:
    raise CRCCheckFailure()

  return output


class Framer:
  def __init__(
      self,
      encode_fn=encode,
      decode_fn=decode,
      crc=CrcSize.CRC8
  ):
    self._encode_fn = encode_fn
    self._decode_fn = decode_fn
    self._crc = crc
    self._buffer = bytearray()
    self._frame = bytearray()

  def encode_frame(self, data: bytes):
    if not data:
      raise EncodeError('data must not be empty')

    if self._crc != CrcSize.NO_CRC:
      data = append_crc(data, crc_size=self._crc)

    return b'\x00' + encode(data) + b'\x00'

  def decode_frame(self) -> Optional[bytearray]:
    while self._buffer:
      byte = self._buffer.pop(0)

      if byte == 0:
        if self._frame:
          try:

            decoded = self._decode_frame_int(self._frame)
            return decoded
          finally:
            self._frame.clear()
      else:
        self._frame.append(byte)

    return None

  def clear_buffer(self):
    self._buffer.clear()
    self._frame.clear()

  def append_buffer(self, data: bytes):
    self._buffer.extend(data)

  def _decode_frame_int(self, data):
    data = decode(data)

    if self._crc != CrcSize.NO_CRC:
      data = check_crc(data, crc_size=self._crc)

    return data
