# Primitive Types
#################
# uint[bits]             unsigned integer
# int[bits]              signed integer
# float[bits]            iEEE floating point number
# flag:                  One bit boolean value
# bits[bits]             Some number of bits,
# bytes[bytes]           Some number of bytes.
# bytes[]                series of bytes, length is variable
# string[bytes]          fixed length string, may be ascii or utf8. Unused characters are indicated as null
# string[]               variable length string
# unused[bits]           indicates data that is not used for any purpose, or reserved

enum Direction: uint[2] {
  Up = 0
  Down = 1
  Left = 2
  Right = 3
}

enum Speed: uint[8] {
  Stopped = 0
  Slow = 80
  Fast = 255
}

struct Move {
  speed: Speed
  direction: Direction
}

struct Ack {
  code: uint[8]
}

protocol {
  maxLength = 256
  version =  5
  minVersion = 2
  framing = COBS
  crc = True

  messageIds {
    Move = 1
    Ack = 2
  } 
}