#include <iostream>
#include <iomanip>
#include "struct.h"
#include "cpptiny.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

using namespace std;
using namespace Bakelite;

string hexString(const char *data, int length, bool space) {
  stringstream ss;
  
  ss << std::hex;

  for(int i = 0; i < length; i++) {
    ss << std::setw(2) << std::setfill('0') << (unsigned int)(data[i] & 0xffu);
    if(space) {
      ss << ' ';
    }
  }
  
  return ss.str();
}

void printHex(const char *data, int length) {
  cout << hexString(data, length, false) << endl;
}

TEST_CASE("simple struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256);

  Ack t1 = {
    123
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 1);
  CHECK(hexString(data, stream.pos()) == "7b");

  Ack t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.code == 123);
}

TEST_CASE("complex struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256);

  TestStruct t1 = {
    5,
    -1234,
    31,
    1234,
    -1.23,
    true,
    true,
    false,
    {1, 2, 3, 4},
    "hey",
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 24);
  CHECK(hexString(data, stream.pos()) == "052efbffff1fd204a4709dbf010100040102030468657900");

  TestStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.int1 == 5);
  CHECK(t2.int2 == -1234);
  CHECK(t2.uint1 == 31);
  CHECK(t2.uint2 == 1234);
  CHECK(t2.float1 == doctest::Approx(-1.23));
  CHECK(t2.b1 == true);
  CHECK(t2.b2 == true);
  CHECK(t2.b3 == false);
  CHECK(string(t2.str) == "hey");
}

TEST_CASE("enum struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256);

  EnumStruct t1 = {
    Direction::Left,
    Speed::Fast
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 2);
  CHECK(hexString(data, stream.pos()) == "02ff");

  EnumStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK_EQ(t2.direction, Direction::Left);
  CHECK_EQ(t2.speed, Speed::Fast);
}

TEST_CASE("nested struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256);

  NestedStruct t1 = {
    {true, false},
    { 127 },
    -4
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 4);
  CHECK(hexString(data, stream.pos()) == "01007ffc");

  NestedStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.a.b1 == true);
  CHECK(t2.a.b2 == false);
  CHECK(t2.b.num == 127);
  CHECK(t2.num == -4);
}

TEST_CASE("deeply nested struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256);

  DeeplyNestedStruct t1 = {
    { { false, true } }
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 2);
  CHECK(hexString(data, stream.pos()) == "0001");

  DeeplyNestedStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.c.a.b1 == false);
  CHECK(t2.c.a.b2 == true);
}

TEST_CASE("struct with arrays") {
  char *data = new char[256];
  BufferStream stream(data, 256);

  ArrayStruct t1;
  t1.a.push_back(Direction::Left);
  t1.a.push_back(Direction::Right);
  t1.a.push_back(Direction::Down);
  t1.b.push_back({127});
  t1.b.push_back({64});
  strcpy(t1.c.data[0], "abc");
  strcpy(t1.c.data[1], "def");
  strcpy(t1.c.data[2], "ghi");
  t1.c.len = 3;
  REQUIRE(t1.pack(stream) == 0);

  // All arrays now have length prefixes
  CHECK(stream.pos() == 20);
  CHECK(hexString(data, stream.pos()) == "03020301027f4003616263006465660067686900");

  ArrayStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);

  CHECK(t2.a[0] == Direction::Left);
  CHECK(t2.a[1] == Direction::Right);
  CHECK(t2.a[2] == Direction::Down);
  CHECK(t2.b[0].code == 127);
  CHECK(t2.b[1].code == 64);
  CHECK(string(t2.c.data[0]) == "abc");
  CHECK(string(t2.c.data[1]) == "def");
  CHECK(string(t2.c.data[2]) == "ghi");
}

TEST_CASE("struct with variable types") {
  char *data = new char[256];
  BufferStream stream(data, 256);
  uint8_t byteData[11] = { 0x68, 0x65, 0x6C, 0x6C, 0x6F,
        0,
        0x57, 0x6F, 0x72, 0x6C, 0x64 };
  uint8_t numbers[4] = { 1, 2, 3, 4 };

  VariableLength t1;
  t1.a.assign(byteData, 11);
  strcpy(t1.b, "This is a test string!");
  t1.c.assign(numbers, 4);
  // d: bytes[16][4] - array of bytes[16]
  t1.d.data[0].assign((uint8_t*)"\x04\x05\x06", 3);
  t1.d.data[1].assign((uint8_t*)"\x07\x08\x09", 3);
  t1.d.len = 2;
  // e: string[16][4] - array of string[16]
  strcpy(t1.e.data[0], "abc");
  strcpy(t1.e.data[1], "def");
  strcpy(t1.e.data[2], "ghi");
  t1.e.len = 3;
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 62);
  CHECK(hexString(data, stream.pos()) == "0b68656c6c6f00576f726c64546869732069732061207465737420737472696e672100040102030402030405060307080903616263006465660067686900");

  VariableLength t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);

  CHECK(t2.a.size() == 11);
  CHECK(vector<uint8_t>(t2.a.data, t2.a.data+t2.a.size()) == vector<uint8_t>(byteData, byteData+11));
  CHECK(string(t2.b) == "This is a test string!");
  CHECK(t2.c.size() == 4);
  CHECK(vector<uint8_t>(t2.c.data, t2.c.data+t2.c.size()) == vector<uint8_t>({1, 2, 3, 4}));
  CHECK(t2.d.size() == 2);
  CHECK(t2.d.data[0].size() == 3);
  CHECK(t2.d.data[0].data[0] == 4);
  CHECK(t2.d.data[0].data[1] == 5);
  CHECK(t2.d.data[0].data[2] == 6);
  CHECK(t2.d.data[1].size() == 3);
  CHECK(t2.d.data[1].data[0] == 7);
  CHECK(t2.d.data[1].data[1] == 8);
  CHECK(t2.d.data[1].data[2] == 9);
  CHECK(t2.e.size() == 3);
  CHECK(string(t2.e.data[0]) == "abc");
  CHECK(string(t2.e.data[1]) == "def");
  CHECK(string(t2.e.data[2]) == "ghi");
}
