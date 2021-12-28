#include "types.h"
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <type_traits>

using namespace std;
using namespace Bakelite;

ostream &printHex(const char *data, int length) {
  std::ios state(NULL);
  state.copyfmt(std::cout);
  for(int i = 0; i < length; i++) {
    cout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(data[i] & 0xffu);
  }
  cout << "\n";
  std::cout.copyfmt(state);
  return std::cout;
}

int main() {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);

  uint8_t arrayVariable[3] = {1, 2, 3};
  const char *stringList[] = { "One", "Two", "Three" };

  TestStruct testStruct = {
    5,
    -1234,
    31,
    1234,
    -1.23,
    true,
    true,
    false,
    {1, 2, 3, 4},
    "Hey",
    {1, 2, 3 , 4, 5},
    { arrayVariable, 3 },
    (char *)"Hello World!",
    { (char **)stringList, 3}
  };
  int rcode = testStruct.pack(stream);
  cout << "Pack: " << rcode << endl;

  cout << stream.pos() << endl;
  printHex(data, stream.pos());

  TestStruct t2;
  stream.seek(0);
  rcode = t2.unpack(stream);
  cout << "Unpack: " << rcode << endl;

  cout
    << "int1: " << (int)t2.int1 << endl
    << "int2: " << t2.int2 << endl
    << "uint1: " << (unsigned)t2.uint1 << endl
    << "uint2: " << t2.uint2 << endl
    << "float1: " << t2.float1 << endl
    << "b1: " << (t2.b1 ? "true" : "false") << endl
    << "b2: " << (t2.b2 ? "true" : "false") << endl
    << "b3: " << (t2.b3 ? "true" : "false") << endl
    << "data: ";
  printHex((const char *)t2.data, sizeof(t2.data));
  cout
    << "str: " << t2.str << endl;
  cout << "array: ";
  printHex((const char *)t2.array, sizeof(t2.array));
  cout << "arrayVariable: ";
  printHex((const char *)t2.arrayVariable.data, t2.arrayVariable.size);
  cout << "stringVariable: '" << t2.stringVariable << "'" << endl;
  for(int i = 0; i < t2.stringList.size; i++) {
    cout << "stringList: '" << t2.stringList.data[i] << "'" << endl;
  }
}