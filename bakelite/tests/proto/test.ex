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
# string[]            variable length string, null terminated
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

##
## Messages
##

# GPIO
struct WritePortConfig {
  portNumber: uint[3]
  config: PortConfiguration
}

struct ReadPortConfig {
  portNumber: uint[3]

}

struct WritePort {
  portNumber: uint[3]
  output: bits[8]
}

struct ReadPort {
  portNumber: uint[3]
}

# SPI
struct ConfigureSPI {
  portNumber: uint[2]
  bitRate: uint[16]
}

struct SpiWriteData { 
  portNumber: uint[2]
  bitRate: uint[16]
}

struct SpiReadData {
  portNumber: uint[2]
  data: bytes[]
}

# ADC
struct StartADC {
  channel: uint[4]
  rate: uint[8]
  chunkSize: uint[8]
}

struct StopADC {
  channel: uint[4]
}

##
## Responses
##

struct Ack {
  code: uint[8]
}

struct PortConfiguration {
  pinConfig: PinCOnfig[8]
}

struct SpiData {
  data: bytes[]
}

struct AdcData {
  data: bytes[]
}

struct InterruptNotify {
  port: int[2]
  pin: int[3]
  pinState: flag
}

##
## Protocl Defenition
##

protocol {
  maxLength = 256
  version =  5
  minVersion = 2
  framing = COBS
  crc = True

  # Option list of messages that can be sent or received
  # Bakelite will automatically encode/decode the messages based on the ID
  # Message IDs need to be stable over time, so they have a statically assigned ID
  messageIds {
    # Commands
    WritePortConfig = 1
    ReadPortConfig = 2
    WritePort = 3
    ReadPort = 4
    ConfigureSPI = 5
    SpiWriteData = 6
    SpiReadData = 7
    StartADC = 8
    StopADC = 9
    
    # Responses
    Ack = 10
    PortConfiguration = 11
    SpiData = 12
    AdcData = 13
    InterruptNotify = 14
  }
}
