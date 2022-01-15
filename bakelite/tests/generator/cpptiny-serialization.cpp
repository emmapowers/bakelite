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
  BufferStream stream(data, 256, heap, 256);

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
  BufferStream stream(data, 256, heap, 256);

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
  CHECK(hexString(data, stream.pos()) == "052efbffff1fd204a4709dbf010100010203046865790000");

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
  BufferStream stream(data, 256, heap, 256);

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
  BufferStream stream(data, 256, heap, 256);

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
  BufferStream stream(data, 256, heap, 256);

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

TEST_CASE("struct with fixed arrays") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);

  ArrayStruct t1 = {
    { Direction::Left, Direction::Right, Direction::Down },
    { { 127 }, { 64 } },
    { "abc", "def", "ghi" }
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 17);
  CHECK(hexString(data, stream.pos()) == "0203017f40616263006465660067686900");

  ArrayStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.a[0] == Direction::Left);
  CHECK(t2.a[1] == Direction::Right);
  CHECK(t2.a[2] == Direction::Down);
  CHECK(t2.b[0].code == 127);
  CHECK(t2.b[1].code == 64);
  CHECK(string(t2.c[0]) == "abc");
  CHECK(string(t2.c[1]) == "def");
  CHECK(string(t2.c[2]) == "ghi");
}

TEST_CASE("struct with variable types") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);
  char byteData[11] = { 0x68, 0x65, 0x6C, 0x6C, 0x6F,
        0,
        0x57, 0x6F, 0x72, 0x6C, 0x64 };
  uint8_t numbers[4] = { 1, 2, 3, 4 };
  char nestedBytesA[3] = { 4, 5, 6 };
  char nestedBytesB[3] = { 7, 8, 9 };
  SizedArray<char> bytesList[2] = {
    { nestedBytesA, 3 },
    { nestedBytesB, 3 },
  };
  const char *stringList[3] = { "abc", "def", "ghi" };

  VariableLength t1 = {
    { byteData, 11 },
    (char *)"This is a test string!",
    { numbers, 4},
    { bytesList, 2 },
    { (char **)stringList, 3 }
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 62);
  CHECK(hexString(data, stream.pos()) == "0b68656c6c6f00576f726c64546869732069732061207465737420737472696e672100040102030402030405060307080903616263006465660067686900");

  VariableLength t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);

  CHECK(t2.a.size == 11);
  CHECK(vector<uint8_t>(t2.a.data, t2.a.data+t2.a.size) == vector<uint8_t>(byteData, byteData+11));
  CHECK(string(t2.b) == "This is a test string!");
  CHECK(t2.c.size == 4);
  CHECK(vector<uint8_t>(t2.c.data, t2.c.data+t2.c.size) == vector<uint8_t>({1, 2, 3, 4}));
  CHECK(t2.d.size == 2);
  CHECK(t2.d.data[0].size == 3);
  CHECK(t2.d.data[0].data[0] == 4);
  CHECK(t2.d.data[0].data[1] == 5);
  CHECK(t2.d.data[0].data[2] == 6);
  CHECK(t2.d.data[1].size == 3);
  CHECK(t2.d.data[1].data[0] == 7);
  CHECK(t2.d.data[1].data[1] == 8);
  CHECK(t2.d.data[1].data[2] == 9);
  CHECK(t2.e.size == 3);
  CHECK(string(t2.e.data[0]) == "abc");
  CHECK(string(t2.e.data[1]) == "def");
  CHECK(string(t2.e.data[2]) == "ghi");
}
