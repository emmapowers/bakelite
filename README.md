# Bakelite

Bakelite is a utility that automates the tedious tasks involved in communicating with hardware.
You define your protocol defenition, and Bakelite will generate source code for you.
Common tasks such as framing and error detection are handled out of the box.

## Features
* Supported languages:
  * C++
  * Python
* Protocol supports:
  * Enums, Structs, strings, binary data, integers and floating point numbers of varying widths.
  * Variable length strings and binary data.
* Framing (COBS)
* Error checking (CRC 8/16/32)

Documentation hasn't been written yet, but a more formal overview of the protocol can be found
[here](./docs/protocol.md).

## Status
This project is in early development. The C++ implementation is currently WIP.
The API and data format are not stable, and will change without notice.
The package has not yet been published to pypi.
If you'd like to try out an early version, see the [contributing](./CONTRIBUTING.md) guide for installation instructions.


# Usage

## Installation

Bakelite requires Python 3.8 or above.

Install it via pip.
```bash
$ pip install bakelite
```
__This is for future reference, it hasn't been published to pypi yet.__
If you'd like to try out an early version, see the [contributing](./CONTRIBUTING.md) guide.

## Code Generation

After installation, a new CLI tool `bakelite` is now available.

Craete a protocol defenition file `my_proto.bakelite`.
```text
struct TestMessage {
  message: string[128]
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

Then generate bindings for the languages you use.

```bash
# Generate C++ Bindings
$ bakelite runtime -l cpptiny -o bakelite.h
$ bakelite gen -l cpptiny -i my_proto.bakelite -o my_proto.h

# Generate Python Bindings
$ bakelite gen -l python -i my_proto.bakelite -o my_proto.py
```
See the [Ardiuno Example](./examples/arduino) for a more complete example.