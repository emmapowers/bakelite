# Quickstart

## Install Bakelite

Bakelite requires Python 3.8 or above.

Install it via pip.
```bash
$ pip install bakelite
```

After installation, a new CLI tool `bakelite` is now available.

## Define the Protocol

Create a protocol definition file `my_proto.bakelite`.
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

## Generate Code

```bash
# Generate C++ code
$ bakelite runtime -l cpptiny -o bakelite.h
$ bakelite gen -l cpptiny -i my_proto.bakelite -o my_proto.h

# Generate Python code
$ bakelite gen -l python -i my_proto.bakelite -o my_proto.py
```