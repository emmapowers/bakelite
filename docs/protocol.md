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
| `bytes[N]` | 1 + data | Variable-length bytes, max N (length-prefixed) |
| `string[N]` | data + 1 | Variable-length string, max N chars (null-terminated) |
| `T[N]` | 1 + elements | Variable-length array, max N elements (length-prefixed) |

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

`bytes[N]` holds up to N bytes, prefixed with a 1-byte length. Any byte value is valid, including null.

```
bytes[8] = [0x05, 0x00, 0xAF, 0xDE] (4 bytes, max 8):
┌──────┬──────┬──────┬──────┬──────┐
│  04  │  05  │  00  │  AF  │  DE  │
└──────┴──────┴──────┴──────┴──────┘
  len    ─────── data ───────
```

### Strings

`string[N]` holds up to N characters, null-terminated. No length prefix or padding is used.

```
string[32] = "Hey":
┌──────┬──────┬──────┬──────┐
│  H   │  e   │  y   │  00  │
└──────┴──────┴──────┴──────┘
```

A `string[N]` can hold at most N characters (one extra byte for null is used on the wire).

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

`T[N]` holds up to N elements, prefixed with a 1-byte length.

```
uint16[8] = [1, 2, 3] (3 elements, max 8):
┌──────┬──────┬──────┬──────┬──────┬──────┬──────┐
│  03  │  01  │  00  │  02  │  00  │  03  │  00  │
└──────┴──────┴──────┴──────┴──────┴──────┴──────┘
  len    ─ [0] ─     ─ [1] ─     ─ [2] ─
```

#### Arrays of Strings or Bytes

For arrays of strings or bytes, the array brackets follow the type size:

- `string[32][5]` — up to 5 strings of max 32 chars each
- `bytes[16][4]` — up to 4 byte arrays of max 16 bytes each

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
              | ("bytes" | "string") "[" NUMBER "]"

array         = "[" NUMBER "]"
annotations   = annotation+
annotation    = "@" NAME ["(" arguments ")"]
```
