#include <iostream>
#include <iomanip>
#include "cpptiny.h"
#include "proto.h"
#include "doctest.h"

using namespace std;
using namespace Bakelite;

class TestStream {
public:
  TestStream(char *buff, uint32_t size,
                char *heap = nullptr, uint32_t heapSize = 0):
    m_buff(buff),
    m_size(size)
  {}

  size_t write(const char *data, uint32_t length) {
    uint32_t endPos = m_pos + length;
    if(endPos > m_size) {
      return -1;
    }

    memcpy(m_buff+m_pos, data, length);
    m_pos += length;

    return length;
  }

  int read() {
    if(m_blocking) {
      return -1;
    }

    uint32_t endPos = m_pos + 1;
    if(endPos > m_size) {
      return -2;
    }
    int ret = (uint8_t)m_buff[m_pos];
    m_pos = endPos;

    return ret;
  }

  int seek(uint32_t pos) {
    if(pos >= m_size) {
      return -3;
    }
    else {
      m_pos = pos;
    }
    return 0;
  }

  uint32_t size() const {
    return m_size;
  }

  uint32_t pos() const {
    return m_pos;
  }

  void setBlocking(bool blocking) {
    m_blocking = blocking;
  }

  void reset() {
    m_blocking = false;
    memset(m_buff, 0, m_size);
    m_pos = 0; 
  }

  string hex() const {
    return hexString(m_buff, m_pos);
  }

private:
  bool m_blocking = false;
  char *m_buff;
  size_t m_size;
  size_t m_pos = 0;
};

char data[256];
TestStream stream(data, 256);

TEST_CASE("Proto send message") {
  stream.reset();
  Protocol protocol(
    []() { return stream.read(); },
    [](const char *data, size_t length) { return stream.write(data, length); }
  );

  Ack ack = {0x22};
  protocol.send(ack);

  CHECK(stream.pos() == 5);
  CHECK(stream.hex() == "040222c400");

  size_t length = stream.pos();
  stream.seek(0);

  for(;stream.pos() < length - 1;) {
    CHECK(protocol.poll() == Protocol::Message::NoMessage);
  }
  auto msg = protocol.poll();
  REQUIRE(msg == Protocol::Message::Ack);

  Ack result;
  CHECK(protocol.decode(result) == 0);
  CHECK(result.code == 0x22);
}

TEST_CASE("Proto send larger message") {
  stream.reset();
  
  // Just for fun, use crc32
  using CustomProtocol = ProtocolBase<CobsFramer<Crc32, 256>>;
  Protocol protocol(
    []() { return stream.read(); },
    [](const char *data, size_t length) { return stream.write(data, length); }
  );
  const char text[16] = "Hello World!";
  TestMessage input = {0x22, -1234, false};
  strncpy(input.message, text, 16);
  protocol.send(input);

  CHECK(stream.pos() == 26);
  CHECK(stream.hex() == "0701222efbffff0d48656c6c6f20576f726c6421010101026200");

  size_t length = stream.pos();
  stream.seek(0);

  for(;stream.pos() < length - 1;) {
    CHECK(protocol.poll() == Protocol::Message::NoMessage);
  }
  auto msg = protocol.poll();
  REQUIRE(msg == Protocol::Message::TestMessage);

  TestMessage result;
  CHECK(protocol.decode(result) == 0);
  CHECK(result.a == 0x22);
  CHECK(result.b == -1234);
  CHECK(result.status == false);
  CHECK(string(result.message, strlen(result.message)) == "Hello World!");
}

TEST_CASE("Proto decode wrong message") {
  stream.reset();
  Protocol protocol(
    []() { return stream.read(); },
    [](const char *data, size_t length) { return stream.write(data, length); }
  );

  Ack ack = {0x22};
  protocol.send(ack);

  CHECK(stream.pos() == 5);
  CHECK(stream.hex() == "040222c400");

  size_t length = stream.pos();
  stream.seek(0);

  for(;stream.pos() < length - 1;) {
    CHECK(protocol.poll() == Protocol::Message::NoMessage);
  }
  auto msg = protocol.poll();
  REQUIRE(msg == Protocol::Message::Ack);

  TestMessage result;
  CHECK(protocol.decode(result) == -1);
}

TEST_CASE("Proto recieve no dynamic memory") {
  stream.reset();
  Protocol protocol(
    []() { return stream.read(); },
    [](const char *data, size_t length) { return stream.write(data, length); }
  );

  ArrayMessage msg;
  int32_t numbers[3] = {1234, -1234, 456};
  msg.numbers.data = numbers;
  msg.numbers.size = 3;
  protocol.send(msg);

  CHECK(stream.pos() == 17);
  CHECK(stream.hex() == "050303d20401072efbffffc8010102bb00");

  size_t length = stream.pos();
  stream.seek(0);

  for(;stream.pos() < length - 1;) {
    CHECK(protocol.poll() == Protocol::Message::NoMessage);
  }
  auto msgId = protocol.poll();
  REQUIRE(msgId == Protocol::Message::ArrayMessage);

  ArrayMessage result;
  CHECK(protocol.decode(result) == -4);
}

TEST_CASE("Proto recieve dynamic") {
  stream.reset();
  Protocol protocol(
    []() { return stream.read(); },
    [](const char *data, size_t length) { return stream.write(data, length); }
  );

  ArrayMessage msg;
  int32_t numbers[3] = {1234, -1234, 456};
  msg.numbers.data = numbers;
  msg.numbers.size = 3;
  protocol.send(msg);

  CHECK(stream.pos() == 17);
  CHECK(stream.hex() == "050303d20401072efbffffc8010102bb00");

  size_t length = stream.pos();
  stream.seek(0);

  for(;stream.pos() < length - 1;) {
    CHECK(protocol.poll() == Protocol::Message::NoMessage);
  }
  auto msgId = protocol.poll();
  REQUIRE(msgId == Protocol::Message::ArrayMessage);

  ArrayMessage result;
  char buffer[16];
  CHECK(protocol.decode(result, buffer, sizeof(buffer)) == 0);
  CHECK(result.numbers.size == 3);
  CHECK(result.numbers.data[0] == 1234);
  CHECK(result.numbers.data[1] == -1234);
  CHECK(result.numbers.data[2] == 456);
}

// Convenience test for checking memory overhead
// TEST_CASE("Proto check size") {
//   stream.reset();
//   Protocol protocol(
//     []() { return stream.read(); },
//     [](const char *data, size_t length) { return stream.write(data, length); }
//   );

//   CHECK(sizeof(protocol) == 0);
// }