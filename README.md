# Bakelite

Bakelite is a utility that automates the tedious tasks involved in communicating with hardware.
You define your protocol defenition, and Bakelite will generate source code for you.


## Features
* Supported Languages
  * C++
  * Python

# Setup

## Requirements

* Python 3.8+

## Installation

Install it via pip.

```text
$ pip install bakelite
```

# Usage

After installation, the package can imported:

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
