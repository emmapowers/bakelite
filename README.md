# Bakelite

[![Tests](https://github.com/emmapowers/bakelite/actions/workflows/test.yaml/badge.svg)](https://github.com/emmapowers/bakelite/actions/workflows/test.yaml)
[![Documentation Status](https://readthedocs.org/projects/bakelite/badge/?version=latest)](https://bakelite.readthedocs.io/en/latest/?badge=latest)

Bakelite is a utility that makes it simple to communicate with your firmware.
Bakelite uses a code generator to automate the tedius process of hand building your own protocol.
It includes features like framing, error detection, and a lightweight C++ implementation suitable for small microcontrollers.

[Documentation](https://bakelite.readthedocs.io/en/latest/)

## Features
* Compact, easy to understand data serialization format
* Simple message passing
* Built in framing and error detection
* Easy to integrate with Serial, USB, TCP, I2C, etc...
* Use only the parts you need
* Code generators for:
    * C++ (header only, no STL or memory allocation)
    * Python

A more formal overview of the protocol can be found
[here](./docs/protocol.md), and examples can be found [here](./examples).

## Status
This project is in early development. The C++ implementation is currently WIP.
The API and data format are not stable, and will change without notice.

# Usage

## Installation

Bakelite requires Python 3.8 or above.

Install it via pip.
```bash
$ pip install bakelite
```

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

A more full features example can be found [here](./examples/arduino).