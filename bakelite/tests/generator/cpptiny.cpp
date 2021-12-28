#include <iostream>
#include <iomanip>
#include "struct.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

using namespace std;
using namespace Bakelite;

string hexString(const char *data, int length) {
  stringstream ss;
  
  ss << std::hex;

  for(int i = 0; i < length; i++) {
    ss << std::setw(2) << std::setfill('0') << (unsigned int)(data[i] & 0xffu);
  }
  
  return ss.str();
}

void printHex(const char *data, int length) {
  cout << hexString(data, length) << endl;
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