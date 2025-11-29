"""Runtime support for bakelite protocol communication."""

import struct
from asyncio import StreamReader, StreamWriter
from collections.abc import AsyncIterator, Coroutine, Iterator
from io import BufferedIOBase
from typing import Any, ClassVar, Literal, overload

from .framing import CrcSize, Framer
from .serialization import Struct


class ProtocolError(RuntimeError):
    """Raised when protocol operations fail."""


class ProtocolBase:
    """Base class for generated protocol classes.

    Supports both synchronous and asynchronous I/O. For sync operations,
    pass a BufferedIOBase stream. For async operations, pass a tuple of
    (StreamReader, StreamWriter).

    Generated subclasses define:
        _message_types: ClassVar[dict[int, type[Struct]]]  # id -> type
        _message_ids: ClassVar[dict[str, int]]  # name -> id

    Example (sync):
        proto = Protocol(stream=serial_port)
        proto.send(MyMessage(data=42))
        for msg in proto.messages():
            handle(msg)

    Example (async):
        reader, writer = await asyncio.open_connection(host, port)
        proto = Protocol(stream=(reader, writer))
        await proto.send(MyMessage(data=42), async_=True)
        async for msg in proto.messages(async_=True):
            await handle(msg)
    """

    _message_types: ClassVar[dict[int, type[Struct]]]
    _message_ids: ClassVar[dict[str, int]]

    _stream: BufferedIOBase | None
    _async_reader: StreamReader | None
    _async_writer: StreamWriter | None

    def __init__(
        self,
        *,
        stream: BufferedIOBase | tuple[StreamReader, StreamWriter],
        crc: str = "CRC8",
        framer: Framer | None = None,
    ) -> None:
        if isinstance(stream, tuple):
            self._stream = None
            self._async_reader, self._async_writer = stream
        else:
            self._stream = stream
            self._async_reader = None
            self._async_writer = None

        crc_size = CrcSize.NO_CRC
        crc_lower = crc.lower()

        if crc_lower == "none":
            crc_size = CrcSize.NO_CRC
        elif crc_lower == "crc8":
            crc_size = CrcSize.CRC8
        elif crc_lower == "crc16":
            crc_size = CrcSize.CRC16
        elif crc_lower == "crc32":
            crc_size = CrcSize.CRC32
        else:
            raise ProtocolError(f"Unknown CRC type {crc}")

        if not framer:
            self._framer = Framer(crc=crc_size)
        else:
            self._framer = framer

    def _encode_message(self, message: Struct) -> bytes:
        """Encode a message to bytes (shared by sync and async)."""
        msg_name = type(message).__name__
        if msg_name not in self._message_ids:
            raise ProtocolError(f"{type(message).__name__} has no message ID")
        msg_id = self._message_ids[msg_name]

        payload = struct.pack("=B", msg_id) + message.pack()
        return self._framer.encode_frame(payload)

    def _decode_frame(self, frame: bytes) -> Struct:
        """Decode a frame to a message (shared by sync and async)."""
        msg_id = frame[0]
        msg_data = frame[1:]

        if msg_id not in self._message_types:
            raise ProtocolError(f"Received unknown message id {msg_id}")

        msg_type = self._message_types[msg_id]
        instance, _ = msg_type.unpack(msg_data)
        return instance

    @overload
    def send(self, message: Struct, *, async_: Literal[False] = False) -> None: ...

    @overload
    def send(self, message: Struct, *, async_: Literal[True]) -> Coroutine[Any, Any, None]: ...

    def send(self, message: Struct, *, async_: bool = False) -> None | Coroutine[Any, Any, None]:
        """Send a message over the protocol stream.

        Args:
            message: The message to send.
            async_: If True, returns a coroutine for async sending.

        Returns:
            None for sync, or a coroutine for async.
        """
        if async_:
            return self._send_async(message)

        if self._stream is None:
            raise ProtocolError("Sync send requires a BufferedIOBase stream")
        frame = self._encode_message(message)
        self._stream.write(frame)
        return None

    async def _send_async(self, message: Struct) -> None:
        """Async implementation of send."""
        if self._async_writer is None:
            raise ProtocolError("Async send requires a StreamWriter")
        frame = self._encode_message(message)
        self._async_writer.write(frame)
        await self._async_writer.drain()

    @overload
    def poll(self, *, async_: Literal[False] = False) -> Struct | None: ...

    @overload
    def poll(self, *, async_: Literal[True]) -> Coroutine[Any, Any, Struct | None]: ...

    def poll(self, *, async_: bool = False) -> Struct | None | Coroutine[Any, Any, Struct | None]:
        """Poll for incoming messages.

        Args:
            async_: If True, returns a coroutine for async polling.

        Returns:
            A message if one is available, None otherwise.
            For async, returns a coroutine.
        """
        if async_:
            return self._poll_async()

        if self._stream is None:
            raise ProtocolError("Sync poll requires a BufferedIOBase stream")
        data = self._stream.read()
        if data:
            self._framer.append_buffer(data)

        frame = self._framer.decode_frame()
        if frame:
            return self._decode_frame(frame)
        return None

    async def _poll_async(self) -> Struct | None:
        """Async implementation of poll."""
        if self._async_reader is None:
            raise ProtocolError("Async poll requires a StreamReader")

        # Read available data (non-blocking read of up to 4096 bytes)
        try:
            data = await self._async_reader.read(4096)
            if data:
                self._framer.append_buffer(data)
        except Exception:
            pass

        frame = self._framer.decode_frame()
        if frame:
            return self._decode_frame(frame)
        return None

    @overload
    def messages(self, *, async_: Literal[False] = False) -> Iterator[Struct]: ...

    @overload
    def messages(self, *, async_: Literal[True]) -> AsyncIterator[Struct]: ...

    def messages(self, *, async_: bool = False) -> Iterator[Struct] | AsyncIterator[Struct]:
        """Iterate over incoming messages.

        This is the primary interface for consuming protocol streams.

        Args:
            async_: If True, returns an async iterator.

        Returns:
            An iterator (sync) or async iterator yielding messages.

        Example (sync):
            for msg in protocol.messages():
                handle(msg)

        Example (async):
            async for msg in protocol.messages(async_=True):
                await handle(msg)
        """
        if async_:
            return self._messages_async()
        return self._messages_sync()

    def _messages_sync(self) -> Iterator[Struct]:
        """Sync iterator over messages."""
        while True:
            msg = self.poll()
            if msg is not None:
                yield msg

    async def _messages_async(self) -> AsyncIterator[Struct]:
        """Async iterator over messages."""
        while True:
            msg = await self.poll(async_=True)
            if msg is not None:
                yield msg
