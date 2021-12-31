#pragma once

#include "bakelite.h"

struct TestMessage {
  uint8_t a;
  int32_t b;
  bool status;
  char message[16];
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = write(stream, a);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, b);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, status);
    if(rcode != 0)
      return rcode;
    rcode = writeString(stream, message, 16);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = read(stream, a);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, b);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, status);
    if(rcode != 0)
      return rcode;
    rcode = readString(stream, message, 16);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


struct Ack {
  uint8_t code;
  char message[250];
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = write(stream, code);
    if(rcode != 0)
      return rcode;
    rcode = writeString(stream, message, 250);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = read(stream, code);
    if(rcode != 0)
      return rcode;
    rcode = readString(stream, message, 250);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


