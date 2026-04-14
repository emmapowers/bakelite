# Protocol Specification

## Overview

Bakelite's protocol definition language lets you describe message structures for embedded communication.

**Design goals:**
- Simple, easily understood on-wire layout
- Suitable for resource-constrained embedded systems
- Low boilerplate

**Non-goals:**
- Not intended as a file interchange format
- Not useful when one side doesn't know the schema

## Types

### Quick Reference

| Type | Size | Description |
|------|------|-------------|
| `int8` | 1 byte | Signed integer |
| `int16` | 2 bytes | Signed integer |
| `int32` | 4 bytes | Signed integer |
| `int64` | 8 bytes | Signed integer |
| `uint8` | 1 byte | Unsigned integer |
| `uint16` | 2 bytes | Unsigned integer |
| `uint32` | 4 bytes | Unsigned integer |
| `uint64` | 8 bytes | Unsigned integer |
| `float32` | 4 bytes | IEEE 754 single precision |
| `float64` | 8 bytes | IEEE 754 double precision |
| `bool` | 1 byte | `true` (1) or `false` (0) |
| `bytes[N]` | N bytes | Fixed-length byte array |
| `bytes[]` | 1 + data | Variable-length bytes (length-prefixed, max 255) |
| `string[N]` | N bytes | Fixed-length null-terminated string |
| `string[]` | data + 1 | Variable-length null-terminated string |
| `T[N]` | N × sizeof(T) | Fixed-length array |
| `T[]` | 1 + elements | Variable-length array (length-prefixed, max 255) |

### Numeric Types

All numeric types are little-endian.

```
uint16 = 0x1234:
┌──────┬──────┐
│  34  │  12  │
└──────┴──────┘
  LSB    MSB
```

```
int32 = -1 (0xFFFFFFFF):
┌──────┬──────┬──────┬──────┐
│  FF  │  FF  │  FF  │  FF  │
└──────┴──────┴──────┴──────┘
```

### Bool

A boolean is a single byte: `0x00` for false, `0x01` for true.

```
bool = true:
┌──────┐
│  01  │
└──────┘
```

### Bytes

#### Fixed Length `bytes[N]`

Contains exactly N bytes. Any byte value is valid, including null.

```
bytes[4] = [0x05, 0x00, 0xAF, 0xDE]:
┌──────┬──────┬──────┬──────┐
│  05  │  00  │  AF  │  DE  │
└──────┴──────┴──────┴──────┘
```

#### Variable Length `bytes[]`

Prefixed with a 1-byte length (max 255 bytes).

```
bytes[] = [0xAA, 0x00, 0xDE] (3 bytes):
┌──────┬──────┬──────┬──────┐
│  03  │  AA  │  00  │  DE  │
└──────┴──────┴──────┴──────┘
  len    ─── data ───
```

### Strings

All strings are null-terminated.

#### Fixed Length `string[N]`

A null-terminated string in an N-byte buffer. Characters after the null terminator are padding (undefined values).

```
string[5] = "Hey":
┌──────┬──────┬──────┬──────┬──────┐
│  H   │  e   │  y   │  00  │  ??  │
└──────┴──────┴──────┴──────┴──────┘
  ─── string ───  null   pad
```

A `string[N]` can hold at most N-1 characters (one byte reserved for null).

#### Variable Length `string[]`

A null-terminated string with no length prefix. The serializer reads until the first null byte.

```
string[] = "Hey":
┌──────┬──────┬──────┬──────┐
│  H   │  e   │  y   │  00  │
└──────┴──────┴──────┴──────┘
```

### Enums

Enums are named integer constants with a specified underlying type.

```
enum PinDirection: uint8 {
  Input = 0
  Output = 1
  Floating = 2
}
```

On the wire, enums are encoded as their underlying type:

```
PinDirection.Output:
┌──────┐
│  01  │
└──────┘
```

### Structs

A struct is a composite type with named members, encoded in declaration order with no padding.

```
struct Sensor {
  id: uint8
  temperature: int16
  active: bool
}
```

```
Sensor { id=1, temperature=256, active=true }:
┌──────┬──────┬──────┬──────┐
│  01  │  00  │  01  │  01  │
└──────┴──────┴──────┴──────┘
  id     temperature   active
         (little-endian)
```

Structs can contain any type including other structs:

```
struct Reading {
  sensor: Sensor
  timestamp: uint32
}
```

### Arrays

#### Fixed Length `T[N]`

N contiguous elements with no length prefix.

```
uint16[3] = [1, 2, 3]:
┌──────┬──────┬──────┬──────┬──────┬──────┐
│  01  │  00  │  02  │  00  │  03  │  00  │
└──────┴──────┴──────┴──────┴──────┴──────┘
  ─ [0] ─     ─ [1] ─     ─ [2] ─
```

#### Variable Length `T[]`

A 1-byte length prefix followed by elements (max 255 elements).

```
uint16[] = [1, 2] (2 elements):
┌──────┬──────┬──────┬──────┬──────┐
│  02  │  01  │  00  │  02  │  00  │
└──────┴──────┴──────┴──────┴──────┘
  len    ─ [0] ─     ─ [1] ─
```

#### Arrays of Variable-Length Types

For arrays of strings or bytes, the array brackets follow the type size:

- `string[32][5]` — 5 fixed-length strings of 32 bytes each
- `string[][5]` — 5 variable-length strings

## Wire Format

### Byte Order

All multi-byte values are little-endian (LSB first).

### Message Frame Layout

With COBS framing and CRC, a message is transmitted as:

```
┌──────┬──────────┬─────────────────┬───────┬──────┐
│ COBS │ Msg ID   │ Message Payload │  CRC  │ 0x00 │
│ byte │ (1 byte) │                 │       │      │
└──────┴──────────┴─────────────────┴───────┴──────┘
```

Example: 6-byte message with COBS and CRC16:

```
┌──────┬──────┬──────────────────────────────┬──────┬──────┬──────┐
│ COBS │  ID  │         Message (6 B)        │  CRC │  CRC │ 0x00 │
└──────┴──────┴──────────────────────────────┴──────┴──────┴──────┘
  1B     1B              6B                     2B (CRC16)   1B
  Total: 11 bytes on wire
```

## Protocol Block

The protocol block configures framing, error checking, and message IDs.

```
struct TestMessage {
  value: uint16
}

struct Ack {
  code: uint8
}

protocol {
  maxLength = 256
  framing = COBS
  crc = CRC8

  messageIds {
    TestMessage = 1
    Ack = 2
  }
}
```

### Message IDs

Each message type needs a unique ID (1–255; 0 is reserved). Only top-level messages need IDs—nested structs don't.

```
struct SubMessage {
  code: uint8
}

struct TestMessage {
  sub: SubMessage
}

protocol {
  messageIds {
    TestMessage = 1  # SubMessage doesn't need an ID
  }
}
```

### Options

| Option | Values | Description |
|--------|--------|-------------|
| `maxLength` | integer | Maximum message payload size in bytes (excludes framing/CRC overhead) |
| `framing` | `COBS`, `None` | Framing algorithm |
| `crc` | `CRC8`, `CRC16`, `CRC32`, `None` | Error detection |

#### COBS Framing

COBS (Consistent Overhead Byte Stuffing) provides reliable framing with low, fixed overhead. Messages are delimited by null bytes. See [Wikipedia](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing) for details.

#### CRC

CRC error detection catches transmission errors. The CRC is computed over the message ID and payload, appended before COBS encoding.

## Grammar Reference

```ebnf
file          = (enum | struct | protocol | comment)+

comment       = "#" <text>

enum          = [annotations] "enum" NAME ":" type "{" enum_value+ "}"
enum_value    = [annotations] NAME "=" NUMBER

struct        = [annotations] "struct" NAME "{" struct_member+ "}"
struct_member = [annotations] NAME ":" type [array] ["=" default]

protocol      = "protocol" "{" (option | messageIds)+ "}"
option        = NAME "=" VALUE
messageIds    = "messageIds" "{" message_id+ "}"
message_id    = NAME "=" NUMBER

type          = primitive | NAME
primitive     = "int8" | "int16" | "int32" | "int64"
              | "uint8" | "uint16" | "uint32" | "uint64"
              | "float32" | "float64" | "bool"
              | ("bytes" | "string") "[" [NUMBER] "]"

array         = "[" [NUMBER] "]"
annotations   = annotation+
annotation    = "@" NAME ["(" arguments ")"]
```
