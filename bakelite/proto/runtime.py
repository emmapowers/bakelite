"""Runtime support for bakelite protocol communication."""

import struct
from asyncio import StreamReader, StreamWriter
from collections.abc import AsyncIterator, Coroutine, Iterator
from dataclasses import is_dataclass
from enum import Enum
from io import BufferedIOBase, BytesIO
from typing import Any, Literal, overload

from .framing import CrcSize, Framer
from .types import ProtocolDescriptor


class ProtocolError(RuntimeError):
    """Raised when protocol operations fail."""


class Registry:
    """Registry of protocol types (structs and enums)."""

    def __init__(self) -> None:
        self.types: dict[str, Any] = {}

    def register(self, name: str, cls: Any) -> None:
        """Register a type by name."""
        self.types[name] = cls

    def get(self, name: str) -> Any:
        """Get a type by name."""
        return self.types[name]

    def is_enum(self, name: str) -> bool:
        """Check if a registered type is an enum."""
        return issubclass(self.types[name], Enum)

    def is_struct(self, name: str) -> bool:
        """Check if a registered type is a struct (dataclass)."""
        return is_dataclass(self.types[name])


class ProtocolBase:
    """Base class for generated protocol classes.

    Supports both synchronous and asynchronous I/O. For sync operations,
    pass a BufferedIOBase stream. For async operations, pass a tuple of
    (StreamReader, StreamWriter).

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

    _stream: BufferedIOBase | None
    _async_reader: StreamReader | None
    _async_writer: StreamWriter | None
    _registry: Registry
    _desc: ProtocolDescriptor
    _options: dict[str, Any]

    def __init__(
        self,
        *,
        stream: BufferedIOBase | tuple[StreamReader, StreamWriter],
        registry: Registry,
        desc: ProtocolDescriptor,
        crc: str = "CRC8",
        framer: Framer | None = None,
        **kwargs: Any,
    ) -> None:
        if isinstance(stream, tuple):
            self._stream = None
            self._async_reader, self._async_writer = stream
        else:
            self._stream = stream
            self._async_reader = None
            self._async_writer = None

        self._registry = registry
        self._desc = desc
        self._options = kwargs

        self._ids = {mid.number: mid.name for mid in self._desc.message_ids}
        self._messages = {mid.name: mid.number for mid in self._desc.message_ids}

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
            raise RuntimeError(f"Unknown CRC type {crc}")

        if not framer:
            self._framer = Framer(crc=crc_size)
        else:
            self._framer = framer

    def _encode_message(self, message: Any) -> bytes:
        """Encode a message to bytes (shared by sync and async)."""
        if not getattr(message, "_desc", None):
            raise ProtocolError(f"{type(message)} is not a message type")

        msg_name = message._desc.name
        if msg_name not in self._messages:
            raise ProtocolError(f"{type(message)} has not been assigned a message ID")
        msg_id = self._messages[msg_name]

        stream = BytesIO()
        stream.write(struct.pack("=B", msg_id))
        message.pack(stream)
        return self._framer.encode_frame(stream.getvalue())

    def _decode_frame(self, frame: bytes) -> Any:
        """Decode a frame to a message (shared by sync and async)."""
        msg_id = frame[0]
        msg = frame[1:]

        if msg_id in self._ids:
            message_type = self._registry.get(self._ids[msg_id])
            return message_type.unpack(BytesIO(msg))
        raise ProtocolError(f"Received unknown message id {msg_id}")

    @overload
    def send(self, message: Any, *, async_: Literal[False] = False) -> None: ...

    @overload
    def send(self, message: Any, *, async_: Literal[True]) -> Coroutine[Any, Any, None]: ...

    def send(self, message: Any, *, async_: bool = False) -> None | Coroutine[Any, Any, None]:
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

    async def _send_async(self, message: Any) -> None:
        """Async implementation of send."""
        if self._async_writer is None:
            raise ProtocolError("Async send requires a StreamWriter")
        frame = self._encode_message(message)
        self._async_writer.write(frame)
        await self._async_writer.drain()

    @overload
    def poll(self, *, async_: Literal[False] = False) -> Any | None: ...

    @overload
    def poll(self, *, async_: Literal[True]) -> Coroutine[Any, Any, Any | None]: ...

    def poll(self, *, async_: bool = False) -> Any | None | Coroutine[Any, Any, Any | None]:
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

    async def _poll_async(self) -> Any | None:
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
    def messages(self, *, async_: Literal[False] = False) -> Iterator[Any]: ...

    @overload
    def messages(self, *, async_: Literal[True]) -> AsyncIterator[Any]: ...

    def messages(self, *, async_: bool = False) -> Iterator[Any] | AsyncIterator[Any]:
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

    def _messages_sync(self) -> Iterator[Any]:
        """Sync iterator over messages."""
        while True:
            msg = self.poll()
            if msg is not None:
                yield msg

    async def _messages_async(self) -> AsyncIterator[Any]:
        """Async iterator over messages."""
        while True:
            msg = await self.poll(async_=True)
            if msg is not None:
                yield msg
