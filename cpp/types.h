#pragma once

#include <cstdint>
#include "bakelite.h"


struct TestStruct {
  int8_t int1;
  int32_t int2;
  uint8_t uint1;
  uint16_t uint2;
  float float1;
  bool b1;
  bool b2;
  bool b3;
  uint8_t data[4];
  char str[5];
  uint16_t array[5]; 
  Bakelite::SizedArray<uint8_t> arrayVariable;
  char *stringVariable;

  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = write(stream, int1);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, int2);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, uint1);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, uint2);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, float1);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, b1);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, b2);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, b3);
    if(rcode != 0)
      return rcode;
    rcode = writeBytes(stream, data, 4);
    if(rcode != 0)
      return rcode;
    rcode = writeString(stream, str, 5);
    if(rcode != 0)
      return rcode;
    rcode = writeArray(stream, array, 5, [](T &stream, uint16_t val) {
      return write(stream, val);
    });
    if(rcode != 0)
      return rcode;
    rcode = writeArray(stream, arrayVariable, [](T &stream, uint8_t val) {
      return write(stream, val);
    });
    if(rcode != 0)
      return rcode;
    return 0;
  }

  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = read(stream, int1);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, int2);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, uint1);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, uint2);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, float1);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, b1);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, b2);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, b3);
    if(rcode != 0)
      return rcode;
    rcode = readBytes(stream, data, 4);
    if(rcode != 0)
      return rcode;
    rcode = readString(stream, str, 5);
    if(rcode != 0)
      return rcode;
    rcode = readArray(stream, array, 5, [](T &stream, uint16_t &val) {
      return read(stream, val);
    });
    rcode = readArray(stream, arrayVariable, [](T &stream, uint8_t &val) {
      return read(stream, val);
    });
    return 0;
  }
};