#include <iostream>
#include <iomanip>
#include <sstream>
#include "struct.h"
#include "doctest.h"
#include "cpptiny.h"

using namespace std;
using namespace Bakelite;

template <typename T>
auto writeFrame(T &framer, const char *data, size_t length, CobsDecodeState lastState = CobsDecodeState::Decoded) {
  for(size_t i = 0; i < (length - 1); i++) {
    auto result = framer.readFrameByte(data[i]);
    CHECK(result.status == CobsDecodeState::NotReady);
  }

  auto result = framer.readFrameByte(data[length - 1]);
  CHECK(result.status == lastState);
  return result;
}


TEST_CASE("encode frame") {
  const int buffSize = 259;
  char buffer[buffSize + COBS_ENCODE_SRC_OFFSET(buffSize)];
  memset(buffer, 0xFF, sizeof(buffer));
  char *srcPtr = buffer + COBS_ENCODE_SRC_OFFSET(buffSize);
  const char srcData[] = "\x00\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\x00\xAA\xBB";
  memcpy(srcPtr, srcData, sizeof(srcData));

  auto result = cobs_encode(buffer, sizeof(buffer), srcPtr, sizeof(srcData)-1);
  CHECK(result.status == 0);
  CHECK(result.out_len == 260);
  
  memset(buffer+result.out_len, 0xFF, sizeof(buffer) - result.out_len);
  CHECK(hexString(buffer, sizeof(buffer)) == "01ffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee0103aabbff");
}

TEST_CASE("encode frame one byte") {
  const int buffSize = 259;
  char buffer[buffSize + COBS_ENCODE_SRC_OFFSET(buffSize)];
  memset(buffer, 0xFF, sizeof(buffer));
  char *srcPtr = buffer + COBS_ENCODE_SRC_OFFSET(buffSize);
  const char srcData[] = "\x22";
  memcpy(srcPtr, srcData, sizeof(srcData));

  auto result = cobs_encode(buffer, sizeof(buffer), srcPtr, sizeof(srcData)-1);
  CHECK(result.status == 0);
  CHECK(result.out_len == 2);
  
  memset(buffer+result.out_len, 0xFF, sizeof(buffer) - result.out_len);
  CHECK(hexString(buffer, result.out_len) == "0222");
}

TEST_CASE("decode frame") {
  char buffer[260];
  memset(buffer, 0xFF, sizeof(buffer));

  buffer[0] = 0x01;
  buffer[1] = 0xFF;
  for(int i = 0; i < 254; i++) {
    buffer[i+2] = 0xEE;
  }
  buffer[256] = 0x01;
  buffer[257] = 0x03;
  buffer[258] = 0xaa;
  buffer[259] = 0xbb;

  auto result = cobs_decode(buffer, sizeof(buffer), buffer, sizeof(buffer));
  CHECK(result.status == 0);
  CHECK(result.out_len == 258);
  
  memset(buffer+result.out_len, 0xFF, sizeof(buffer) - result.out_len);
  //printHex(buffer, sizeof(buffer));

  CHECK(hexString(buffer, sizeof(buffer)) == "00eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee00aabbffff");
}

TEST_CASE("cobs framer encode") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.encodeFrame("\x11\x22\x33\x44", 4);
  CHECK(result.status == 0);
  CHECK(result.length == 6);
  CHECK(hexString((const char *)result.data, result.length) == "051122334400");
}

TEST_CASE("cobs framer encode zero length") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.encodeFrame("", 0);
  CHECK(result.status == 0);
  CHECK(result.length == 2);
  CHECK(hexString((const char *)result.data, result.length) == "0100");
}

TEST_CASE("cobs framer decode") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = writeFrame(framer, "\x05\x11\x22\x33\x44\x00", 6);
  CHECK(result.length == 4);
  CHECK(hexString((const char *)result.data, result.length) == "11223344");
}

TEST_CASE("cobs framer encode one byte") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.encodeFrame("\x22", 1);
  CHECK(result.status == 0);
  CHECK(result.length == 3);
  CHECK(hexString((const char *)result.data, result.length) == "022200");
}

TEST_CASE("cobs framer decode zero length") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.readFrameByte(0x01);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x00);
  CHECK(result.status == CobsDecodeState::Decoded);
  CHECK(result.length == 0);
  CHECK(hexString((const char *)result.data, result.length) == "");
}

TEST_CASE("cobs framer decode more bytes") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = writeFrame(framer, "\x05\x11\x22\x33\x44\x00", 6);
  CHECK(result.length == 4);
  
  result = writeFrame(framer, "\x05\x11\x22\x33\x44\x00", 6);
  CHECK(result.length == 4);
  CHECK(hexString((const char *)result.data, result.length) == "11223344");

  result = framer.readFrameByte(0x05);
  CHECK(result.status == CobsDecodeState::NotReady);
}

TEST_CASE("cobs framer decode two null bytes") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = writeFrame(framer, "\x05\x11\x22\x33\x44\x00", 6);
  CHECK(result.length == 4);
  CHECK(hexString((const char *)result.data, result.length) == "11223344");

  result = framer.readFrameByte(0x00);
  CHECK(result.status == CobsDecodeState::DecodeFailure);
}

TEST_CASE("cobs buffer overrun") {
  CobsFramer<CrcNoop, 2> framer;
  auto result = framer.readFrameByte(0x05);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x11);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x22);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x33);
  CHECK(result.status == CobsDecodeState::BufferOverrun);
}

TEST_CASE("cobs decode failure") {
  CobsFramer<CrcNoop, 256> framer;
  writeFrame(framer, "\x01\x11\x22\x33\x44\x00", 6, CobsDecodeState::DecodeFailure);
}

TEST_CASE("cobs decode failure 2") {
  CobsFramer<CrcNoop, 256> framer;
  writeFrame(framer, "\x10\x11\x22\x33\x44\x00", 6, CobsDecodeState::DecodeFailure);
}

TEST_CASE("cobs framer roundtrip") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.encodeFrame("\x11\x22\x33\x44", 4);
  REQUIRE(result.status == 0);
  for(int i = 0; i < result.length - 1; i++) {
    auto decodeResult = framer.readFrameByte(result.data[i]);
    CHECK(decodeResult.status == CobsDecodeState::NotReady);
  }

  auto decodeResult = framer.readFrameByte(result.data[result.length - 1]);
  CHECK(decodeResult.status == CobsDecodeState::Decoded);
  CHECK(decodeResult.length == 4);
  CHECK(hexString((const char *)decodeResult.data, decodeResult.length) == "11223344");
}

TEST_CASE("cobs encode framer crc8") {
  CobsFramer<Crc8, 256> framer;
  auto result = framer.encodeFrame("\x11\x22\x33\x44", 4);
  CHECK(result.status == 0);
  CHECK(result.length == 7);
  CHECK(hexString((const char *)result.data, result.length) == "0611223344f900");
}

TEST_CASE("cobs encode framer crc16") {
  CobsFramer<Crc16, 256> framer;
  auto result = framer.encodeFrame("\x11\x22\x33\x44", 4);
  CHECK(result.status == 0);
  CHECK(result.length == 8);
  CHECK(hexString((const char *)result.data, result.length) == "0711223344b1f500");
}

TEST_CASE("cobs encode framer crc32") {
  CobsFramer<Crc32, 256> framer;
  auto result = framer.encodeFrame("\x11\x22\x33\x44", 4);
  CHECK(result.status == 0);
  CHECK(result.length == 10);
  CHECK(hexString((const char *)result.data, result.length) == "0911223344d19df27700");
}

TEST_CASE("cobs decode crc8") {
  CobsFramer<Crc8, 256> framer;
  writeFrame(framer, "\x06\x11\x22\x33\x44\xf9\x00", 7);
}

TEST_CASE("cobs decode crc8 failure") {
  CobsFramer<Crc8, 256> framer;
  writeFrame(framer, "\x06\xFF\x22\x33\x44\xf9\x00", 7, CobsDecodeState::CrcFailure);
}

TEST_CASE("cobs decode crc16") {
  CobsFramer<Crc16, 256> framer;
  writeFrame(framer, "\x07\x11\x22\x33\x44\xb1\xf5\x00", 8);
}

TEST_CASE("cobs decode crc16 failure") {
  CobsFramer<Crc16, 256> framer;
  writeFrame(framer, "\x07\xFF\x22\x33\x44\xb1\xf5\x00", 8 , CobsDecodeState::CrcFailure);
}

TEST_CASE("cobs decode crc32") {
  CobsFramer<Crc32, 256> framer;
  writeFrame(framer, "\x09\x11\x22\x33\x44\xd1\x9d\xf2\x77\x00", 10);
}

TEST_CASE("cobs decode crc32 failure") {
  CobsFramer<Crc32, 256> framer;
  writeFrame(framer, "\x09\xFF\x22\x33\x44\xd1\x9d\xf2\x77\x00", 10, CobsDecodeState::CrcFailure);
}
