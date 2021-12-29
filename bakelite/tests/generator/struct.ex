enum Direction: uint8 {
  Up = 0
  Down = 1
  Left = 2
  Right = 3
}

enum Speed: uint8 {
  Stopped = 0
  Slow = 80
  Fast = 255
}

struct EnumStruct {
  direction: Direction
  speed: Speed
}

struct TestStruct {
  int1: int8
  int2: int32
  uint1: uint8
  uint2: uint16
  float1: float32
  b1: bool
  b2: bool
  b3: bool
  data: bytes[4]
  str: string[5]
}

struct Ack {
  code: uint8
}

struct SubA {
  b1: bool
  b2: bool
}

struct SubB {
  num: uint8
}

struct NestedStruct {
  a: SubA
  b: SubB
  num: int8
}

struct SubC {
  a: SubA
}

struct DeeplyNestedStruct {
  c: SubC
}

struct ArrayStruct {
  a: Direction[3]
  b: Ack[2]
  c: string[4][3]
}

struct VariableLength {
  a: bytes[]
  b: string[]
  c: uint8[]
  d: bytes[][]
  e: string[][]
}