from typing import Callable, Optional

from .crc import CrcSize, crc_funcs


class FrameError(RuntimeError):
  pass


class EncodeError(FrameError):
  pass


class DecodeError(FrameError):
  pass


class CRCCheckFailure(FrameError):
  pass


def _append_block(block: bytearray, output: bytearray) -> None:
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


def decode(data: bytes) -> bytes:
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

  return bytes(output)


def append_crc(data: bytes, crc_size: CrcSize = CrcSize.CRC8) -> bytes:
  return data + crc_funcs[crc_size](data).to_bytes(crc_size.value, byteorder='little')


def check_crc(data: bytes, crc_size: CrcSize = CrcSize.CRC8) -> bytes:
  if not data:
    raise CRCCheckFailure()

  crc_val = int.from_bytes(data[-crc_size.value:], byteorder='little')
  output = data[:-crc_size.value]

  if crc_funcs[crc_size](output) != crc_val:
    raise CRCCheckFailure()

  return output


class Framer:
  def __init__(
      self,
      encode_fn: Callable[[bytes], bytes] = encode,
      decode_fn: Callable[[bytes], bytes] = decode,
      crc: CrcSize = CrcSize.CRC8
  ):
    self._encode_fn = encode_fn
    self._decode_fn = decode_fn
    self._crc = crc
    self._buffer = bytearray()
    self._frame = bytearray()

  def encode_frame(self, data: bytes) -> bytes:
    if not data:
      raise EncodeError('data must not be empty')

    if self._crc != CrcSize.NO_CRC:
      data = append_crc(data, crc_size=self._crc)

    return b'\x00' + encode(data) + b'\x00'

  def decode_frame(self) -> Optional[bytes]:
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

  def clear_buffer(self) -> None:
    self._buffer.clear()
    self._frame.clear()

  def append_buffer(self, data: bytes) -> None:
    self._buffer.extend(data)

  def _decode_frame_int(self, data: bytes) -> bytes:
    data = decode(data)

    if self._crc != CrcSize.NO_CRC:
      data = check_crc(data, crc_size=self._crc)

    return data
