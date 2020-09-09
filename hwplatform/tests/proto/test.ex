# Primitive Types
#################
# uint[bits]          unsigned integer
# int[bits]           signed integer
# float[bits]         iEEE floating point number
# flag:               One bit boolean value
# bits[bits]          Some number of buts,
# bytes[bytes]        Some number of bytes.
# bytes[]             series of bytes, length is variable
# string[bytes]       fixed length string, may be ascii or utf8. Unused characters are indicated as null
# string[]            variable length string
# wstring[characters] fixed length unicode (utf-16) string
# wstring[]           variable length unicode (utf-16) string
# unused[bits]        indicates data that is not used for any purpose, or reserved
#
# ACK                 special type used to indicate a command should only expect a surccess response

enum PinMode: uint[2] {
  Input = 1
  Ouput = 2
  HighZ = 3
  Unkown = 0
}

enum PinPullup: uint[2] {
  Up = 1
  Down = 2
  None = 3
  Unkown = 0
}

enum InteruptMode: uint[2] {
  High = 1
  Low = 2
  HighAndLow = 3
  None = 0
}

struct PinConfig {
  direction: PinMode
  pullup: PinPullup
  @version(3)
  interrupt: InteruptMode
}

struct PortConfiguration {
  pinConfig: PinCOnfig[8]
}

protocol {
  maxLength = 256
  version =  5
  minVersion = 2
  framing = COBS
  crc = True
  commands {
    # GPIO
    writePortConfig(portNumber: uint[3], config: PortConfiguration): ACK
    readPortConfig(portNumber: uint[3]): PortConfiguration
    writePort(output: bits[8]): ACK
    readPort(output): bits[8]

    # SPI
    configureSPI(portNumber: uint[2], bitRate: uint[16]): ACK
    writeData(portNumber: uint[2], bitRate: uint[16]): ACK
    readData(portNumber: uint[2], data: bytes[]): bytes[]

    # ADC
    startADC(channel: uint[4], rate: uint[8], chunkSize: uint: [8]): ACK
    stopADC(channel: uint[4]): ACK
  }
  events {
    adcData[data: bytes[]]
    interupt(port: int[2], pin: int[3], pinState: flag)
  }
}
