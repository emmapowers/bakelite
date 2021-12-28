#pragma once

#include "bakelite.h"

enum class Direction: uint8_t {
  Up = 0,
  Down = 1,
  Left = 2,
  Right = 3,
};


enum class Speed: uint8_t {
  Stopped = 0,
  Slow = 80,
  Fast = 255,
};


struct EnumStruct {
  Direction direction;
  Speed speed;
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = write(stream, (Direction)direction);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, (Speed)speed);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = read(stream, (Direction)direction);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, (Speed)speed);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


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
  char* stringVariable;
  Bakelite::SizedArray<char*> stringList;
  
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
    rcode = writeArray(stream, array, 5, [](T &stream, uint16_t const &val) {
      return write(stream, val);
    });
    if(rcode != 0)
      return rcode;
    rcode = writeArray(stream, arrayVariable, [](T &stream, uint8_t const &val) {
      return write(stream, val);
    });
    if(rcode != 0)
      return rcode;
    rcode = writeString(stream, stringVariable);
    if(rcode != 0)
      return rcode;
    rcode = writeArray(stream, stringList, [](T &stream, char* const &val) {
      return writeString(stream, val);
    });
    if(rcode != 0)
      return rcode;
    return rcode;
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
    if(rcode != 0)
      return rcode;
    rcode = readArray(stream, arrayVariable, [](T &stream, uint8_t &val) {
      return read(stream, val);
    });
    if(rcode != 0)
      return rcode;
    rcode = readString(stream, stringVariable);
    if(rcode != 0)
      return rcode;
    rcode = readArray(stream, stringList, [](T &stream, char* &val) {
      return readString(stream, val);
    });
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


struct Ack {
  uint8_t code;
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = write(stream, code);
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
    return rcode;
  }
};


struct SubA {
  bool b1;
  bool b2;
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = write(stream, b1);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, b2);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = read(stream, b1);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, b2);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


struct SubB {
  uint8_t num;
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = write(stream, num);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = read(stream, num);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


struct NestedStruct {
  SubA a;
  SubB b;
  int8_t num;
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = a.pack(stream);
    if(rcode != 0)
      return rcode;
    rcode = b.pack(stream);
    if(rcode != 0)
      return rcode;
    rcode = write(stream, num);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = a.unpack(stream);
    if(rcode != 0)
      return rcode;
    rcode = b.unpack(stream);
    if(rcode != 0)
      return rcode;
    rcode = read(stream, num);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


struct SubC {
  SubA a;
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = a.pack(stream);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = a.unpack(stream);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


struct DeeplyNestedStruct {
  SubC c;
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = c.pack(stream);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = c.unpack(stream);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


struct ArrayStruct {
  Direction a[3];
  Ack b[2];
  char c[3][4];
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = writeArray(stream, a, 3, [](T &stream, Direction const &val) {
      return write(stream, (Direction)val);
    });
    if(rcode != 0)
      return rcode;
    rcode = writeArray(stream, b, 2, [](T &stream, Ack const &val) {
      return val.pack(stream);
    });
    if(rcode != 0)
      return rcode;
    rcode = writeArray(stream, c, 3, [](T &stream, char* const &val) {
      return writeString(stream, val, 4);
    });
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = readArray(stream, a, 3, [](T &stream, Direction &val) {
      return read(stream, (Direction)val);
    });
    if(rcode != 0)
      return rcode;
    rcode = readArray(stream, b, 2, [](T &stream, Ack &val) {
      return val.unpack(stream);
    });
    if(rcode != 0)
      return rcode;
    rcode = readArray(stream, c, 3, [](T &stream, char* &val) {
      return readString(stream, val, 4);
    });
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};


struct VariableLength {
  Bakelite::SizedArray<uint8_t> a;
  char* b;
  Bakelite::SizedArray<uint8_t> c;
  Bakelite::SizedArray<Bakelite::SizedArray<uint8_t> > d;
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = writeBytes(stream, a);
    if(rcode != 0)
      return rcode;
    rcode = writeString(stream, b);
    if(rcode != 0)
      return rcode;
    rcode = writeArray(stream, c, [](T &stream, uint8_t const &val) {
      return write(stream, val);
    });
    if(rcode != 0)
      return rcode;
    rcode = writeArray(stream, d, [](T &stream, Bakelite::SizedArray<uint8_t> const &val) {
      return writeBytes(stream, val);
    });
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  template<class T>
  int unpack(T &stream) {
    int rcode = 0;
    rcode = readBytes(stream, a);
    if(rcode != 0)
      return rcode;
    rcode = readString(stream, b);
    if(rcode != 0)
      return rcode;
    rcode = readArray(stream, c, [](T &stream, uint8_t &val) {
      return read(stream, val);
    });
    if(rcode != 0)
      return rcode;
    rcode = readArray(stream, d, [](T &stream, Bakelite::SizedArray<uint8_t> &val) {
      return readBytes(stream, val);
    });
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};



