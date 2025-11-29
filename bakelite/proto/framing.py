"""COBS framing and CRC support for bakelite protocols."""

from collections.abc import Callable

from .crc import CrcSize, crc_funcs


class FrameError(RuntimeError):
    """Base exception for framing errors."""


class EncodeError(FrameError):
    """Raised when frame encoding fails."""


class DecodeError(FrameError):
    """Raised when frame decoding fails."""


class CRCCheckFailure(FrameError):
    """Raised when CRC validation fails."""


def _append_block(block: bytearray, output: bytearray) -> None:
    output.append(len(block) + 1)
    output.extend(block)
    block.clear()


def encode(data: bytes) -> bytes:
    """Encode data using COBS (Consistent Overhead Byte Stuffing)."""
    block = bytearray()
    output = bytearray()
    full_block = False

    if not data:
        return b""

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

    return bytes(output)


def decode(data: bytes) -> bytes:
    """Decode COBS-encoded data."""
    output = bytearray()

    if not data:
        return b""

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
    """Append a CRC checksum to data."""
    return data + crc_funcs[crc_size](data).to_bytes(crc_size.value, byteorder="little")


def check_crc(data: bytes, crc_size: CrcSize = CrcSize.CRC8) -> bytes:
    """Verify and strip CRC checksum from data."""
    if not data:
        raise CRCCheckFailure()

    crc_val = int.from_bytes(data[-crc_size.value :], byteorder="little")
    output = data[: -crc_size.value]

    if crc_funcs[crc_size](output) != crc_val:
        raise CRCCheckFailure()

    return output


class Framer:
    """Handles framing and deframing of protocol messages."""

    def __init__(
        self,
        encode_fn: Callable[[bytes], bytes] = encode,
        decode_fn: Callable[[bytes], bytes] = decode,
        crc: CrcSize = CrcSize.CRC8,
    ) -> None:
        self._encode_fn = encode_fn
        self._decode_fn = decode_fn
        self._crc = crc
        self._buffer = bytearray()
        self._frame = bytearray()

    def encode_frame(self, data: bytes) -> bytes:
        """Encode a frame with framing and optional CRC."""
        if not data:
            raise EncodeError("data must not be empty")

        if self._crc != CrcSize.NO_CRC:
            data = append_crc(data, crc_size=self._crc)

        return b"\x00" + encode(data) + b"\x00"

    def decode_frame(self) -> bytes | None:
        """Attempt to decode a complete frame from the buffer."""
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
        """Clear the receive buffer."""
        self._buffer.clear()
        self._frame.clear()

    def append_buffer(self, data: bytes) -> None:
        """Append data to the receive buffer."""
        self._buffer.extend(data)

    def _decode_frame_int(self, data: bytes) -> bytes:
        data = decode(data)

        if self._crc != CrcSize.NO_CRC:
            data = check_crc(data, crc_size=self._crc)

        return data
