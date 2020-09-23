
include "testfile.txt"

enum Whatever: int[16] {
  one,
  two = 3,
  three
}

struct MyStruct {
  testFlag:  flag
  testEnum:  Whatever,
  number1:   int[32],
  number2:   int[8],
  number3:   uint[4],
  number4:   uint[16],
  number5:   float[32],
  number6:   float[64],
  data1:     bits[3],
  data2:     bytes[64],
  data3:     bytes[],
  str1:      string[8],
  str2:      string[],
  str3:      wstring[16],
  @minVersion(5)  
  str4:      wstring[],
}


protocol {
  maxLength 256
  version: 5
  minVersion: 2
  crc: True
  commands {
    example(int[10]): ACK
    example2(MyStruct): ACK
  }
  events {
    oops: MyStruct
  }
}