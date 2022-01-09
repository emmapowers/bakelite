# Protocol Definition

## Design Goals
* Simple
* Easily understood on-wire layout
* Composable
* Suitable for use on resource constrained embedded systems
* Low boilerplate

## Design Non-Goals
* Not intended for use as a file interchange format
* Not useful in situations where one side doesn't know the schema

## Primitive Types
### Numeric Types
|Name                          |Size (Bytes)  |Description            |
|------------------------------|--------------|-----------------------|
|int8, int16, int32, int64     |8, 16, 32, 64 | Signed integer        |
|uint8, uint16, uint32, uint64 |8, 16, 32, 64 | Unsigned integer      |
|float32, float64              |32, 64        | Floating point number |
|bool                          |1             | true/false value      |


### Variable Length
|Name     |Size (Bytes)|Description|
|---------|------------|-----------|
|bytes    |Fixes, N+1  | A list of uint8s |
|string   |Fixed, N+1  | A null-terminated list of uint8s, with optional encoding (ascii or utf8) |

#### Bytes (Fixed Length) `bytes[n]`
Fixed length byte types contain a defined number of bytes.
Each byte can have any value, including NULL.

For Example:
<table>
<tr><td>Type</td><td colspan=4>bytes[4]</td></tr>
<tr><td>Byte</td><td>1</td><td>2</td><td>3</td><td>4</td></tr>
<tr><td>Value</td><td>0x05</td><td>0x00</td><td>0xAF</td><td>0xDE</td></tr>
</table>

#### Bytes (Variable Length) `bytes[]`
Variable length byte arrays may be any size up to 255 bytes.
When one of these fields is serialized, the first byte indicates the size,
while the remaining bytes contain the data.

For example:
<table>
<tr><td>Type</td><td colspan=4>bytes[]</td></tr>
<tr><td>Byte</td><td>1</td><td>2</td><td>3</td><td>4</td></tr>
<tr><td>Value</td><td>0x03</td><td>0xAA</td><td>0x00</td><td>0xDE</td></tr>
<tr><td>Purpose</td><td>Size</td><td colspan=3>Data</td></tr>
</table>

#### String (Fixed Length) `string[n]`
Strings are always null-terminated.
If a string has a fixed length, any characters after the null-terminator may have any value.

For example:
<table>
<tr><td>Type</td><td colspan=5>string[5]</td></tr>
<tr><td>Byte</td><td>1</td><td>2</td><td>3</td><td>4</td><td>5</td></tr>
<tr><td>Value</td><td>H</td><td>e</td><td>y</td><td>0x00</td><td>0x00</td></tr>
</table>

Strings must always contain a null byte.
The below example is invalid, because there is no room to store the null byte.
<table>
<tr><td>Type</td><td colspan=5>string[5]</td></tr>
<tr><td>Byte</td><td>1</td><td>2</td><td>3</td><td>4</td><td>5</td></tr>
<tr><td>Value</td><td>H</td><td>e</td><td>l</td><td>l</td><td>o</td></tr>
</table>

#### String (Variable Length) `string[]`
Unlike `bytes[]` variable lengths strings do not contain a size byte.
Instead, the serializer will read until it encounters the first null byte.

For example:
<table>
<tr><td>Type</td><td colspan=4>string[]</td></tr>
<tr><td>Byte</td><td>1</td><td>2</td><td>3</td><td>4</td></tr>
<tr><td>Value</td><td>H</td><td>e</td><td>y</td><td>0x00</td></tr>
</table>

### Bitfield Types
__Not for V1__

|Name       |Size (Bits)|Description                 |
|-----------|-----------|----------------------------|
|uint{N}    |N bits     | Unsigned integer           |
|int{N}     |N bits     | Signed integer             |
|flag       |1 bit      | True/False value           |
|unused{N}  |N bit      | reserved or padding bits   |

All types have a maximum length of 64 bits.

### Enums
Enums are a set of named constants that help make a protocol definition easier to understand.
For example:
```
enum PinDirection: uint8 {
  Input = 0
  Output = 1
  Floating = 2
}
```
Enums are always numerical, and you must specify the underlying type when declaring the enum.
The enum above is stored using a `uint8`.

### Struct
A struct is a composite data type made of named members of other data types.
For example, this struct contains three members, each with a different type.
```
struct MyTestStruct {
  a: int32,
  b: boolean,
  c: uint16
}
```
Structs can contain any primitive type, bytes, string, arrays, enums, bitfield structs and, other structs.
They cannot contain bitfield types (unless using an embedded bitfield).
The members of a struct are encoded in order.
There is no type or size information encoded with the struct.

For example:
<table>
<tr><td>Type</td><td colspan=7>MyTestStruct</td></tr>
<tr><td>Byte</td><td>1</td><td>2</td><td>3</td><td>4</td><td>5</td><td>6</td><td>7</td></tr>
<tr><td>Member</td><td colspan=4>a</td><td>b</td><td colspan=2>c</td></tr>
</table>

### Bitfield Structs
__Not for V1__

A bitfield struct lets you store data that is not byte aligned.
These can be used in situations where memory or network bandwidth is very tightly constrained or when communicating with register-based hardware.
I2C devices, for example.

Below is an example of a bitfield definition for the control register of a TMP1075 temperature sensor.
```
enum ConversionRate: uint{2} {
  Rate_27_5  = 0  # 27.5ms
  Rate_55    = 1  # 55ms
  Rate_110   = 2  # 110ms
  Rate_220   = 3  # 220ms
}

bitfield struct ControlRegister {
  oneShot: flag
  conversionRate: ConversionRate
  conversionFaults: uint{2}
  outputPriority: flag
  alertFunction: flag
  shutdownMode: flag
  reserved: unused{8}
}
```

#### Embedded bitfields
Bitfield types are not allowed in structs.
You can include bitfield structs as members of a standard struct (see limitations below).
Sometimes though, this can become verbose.
In cases where you only need a few non-byte aligned fields in an otherwise normal struct,
embedded bitfields may be useful.

For example, this struct uses an embedded bitfield to represent the states of 8 pins in a port.
```
struct WritePort {
  portNum: uint8
  bitfield {
    pin1: flag
    pin2: flag
    pin3: flag
    pin4: flag
    pin5: flag
    pin6: flag
    pin7: flag
    pin8: flag
  }
}
```

#### Limitations
Due to their flexibility, bitfield structs may have a size that's not an even multiple of 8bit.
This causes many non-byte aligned reads.
Non-byte aligned reads are size efficient but are expensive to serialize.
To use a bitfield in a normal struct (embedded or otherwise), it must be a multiple of 8 bits.
This limitation also applies when directly serializing a bitfield struct.
If your bitfield struct is not naturally a multiple of 8 bits, the `unused{n}` data type can be used to pad the data to the correct size.

### Arrays
Arrays are created by appending square brackets to the end of a type.
`[n]` for fixed length arrays, `[]` for variable-length arrays.
There is some ambiguity when using arrays of strings or bytes.
In such cases, the array indicator is placed after the type size.
For example, a list of 5 variable-length strings would look like this `string[][5]`.

#### Fixed Length
Fixed-length arrays always contain all their elements.
For example, an array of type `uint16[3]` will be encoded like this:

<table>
<tr><td>Type</td><td colspan=6>uint16[3]</td></tr>
<tr><td>Byte</td><td>1</td><td>2</td><td>3</td><td>4</td><td>5</td><td>6</td></tr>
<tr><td>Element</td><td colspan=2>1</td><td colspan=2>2</td><td colspan=2>3</td></tr>
</table>

#### Variable Length
Variable-length arrays are encoded with a length byte, followed by N elements.
For example:

<table>
<tr><td>Type</td><td colspan=7>uint16[]</td></tr>
<tr><td>Byte</td><td>1</td><td>2</td><td>3</td><td>4</td><td>5</td><td>6</td><td>7</td></tr>
<tr><td>Element</td><td>Size</td><td colspan=2>1</td><td colspan=2>2</td><td colspan=2>...</td></tr>
</table>


### Choice Types
__Not for V1__

### Union Types
__Not for V1__

## Endianness
__Not for V1__

This will wait for a future version, but a default endianness will be assumed.
It can then be changed on a protocol, struct, or field level.

## Framing
A few different framing types will be supported. For the first version, only COBS will be implemented.

### Fixed
__Not for V1__

Frames messages into fixed-length packets.
Use this when your underlying hardware can reliably deliver corruption-free fixed-length messages.

This is not suitable for serial or TCP links.

### Length Based
__Not for V1__

A simple framing method where a 4-byte length prefix is added to the data before it is transmitted.
It's a lightweight protocol suitable for situations where the underlying stack provides error correction.

This would be suitable for use with TCP, or with serial protocols that have well-defined frame boundaries.
It is not suitable for Serial since you may start reading in the middle of a packet.

### COBS
COBS is a robust framing algorithm with low, fixed overhead.
It is also computationally efficient.
Combined with CRC error checking, it is robust against corruption and synchronization loss. See the [Wikipedia article](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing) for more details.

COBS is suitable for serial protocols.

#### Error Checking
CRC8, 16, and 32 are supported.
CRC checks can be disabled if the link is reliable.
Future versions may implement other error detection/correction schemes.

## Protocol
The protocol wraps structs, framing, CRC, and other settings into an object the code generator can use to generate helpers.
The protocol works by sending and receiving messages.
Messages are simply structs that have been assigned a message ID.
When sent, each messages is wrapped in a frame, and proceeded by a message ID byte.
See the [layout](#layout) section below for more details.

The below protocol would generate an implementation that uses COBS framing, CRC8, and supported up to 256 byte messages. It supports two messages `TestMessage` and `Ack`.
```proto
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
To send a struct, it must be assigned a message ID.
Message IDs are used by the receiver to determine which struct to decode.
If a struct is nested inside another struct, only the root struct needs to be assigned an ID.

In the example below, we only intend to send TestMessage, so it is assigned the ID of one.
SubMessage is never sent directly, so no ID is assigned.
```proto
struct SubMessage {
  code: uint8
  text: string[]
}

TestMessage {
  data: SubMessage
  success: bool
}

protocol {
  ...
  messageIds {
    TestMessage = 1
  }
}
```

### Max Length
The `maxLength` parameter describes the maximum size, in bytes, of a message.
This does not include the framing overhead, CRC, or message ID byte.
For example, a maxLength of 64 means that any struct with a size of up to 64 bytes can be sent.

The length of the frame that is actually sent will be longer.
For the above example, if we sent a struct that was 64 bytes in size using COBS framing and CRC16 error detection, then the on-wire frame size would be 69 bytes.

### Layout
Here's an example frame with an 6 byte message, COBS framing and CRC16 error checking.
<table>
<tr>
  <td>Role</td>
  <td colspan=1>Framing</td>
  <td colspan=7>Message</td>
  <td colspan=3>Framing</td>
</tr>
<tr>
  <td>Byte</td>
  <td>1</td><td>2</td><td>3</td><td>4</td><td>5</td><td>6</td>
  <td>7</td><td>8</td><td>9</td><td>10</td><td>11</td>
</tr>
<tr>
  <td>Element</td>
  <td colspan=1>COBS Overhead</td>
  <td colspan=1>Message Id</td>
  <td colspan=6>Message</td>
  <td colspan=2>CRC16</td>
  <td colspan=1>Null Byte</td>
</tr>
</table>