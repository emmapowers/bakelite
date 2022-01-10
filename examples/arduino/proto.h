#pragma once

#include "bakelite.h"

struct TestMessage {
  uint8_t a;
  int32_t b;
  bool status;
  char message[16];
  
  int pack(Bakelite::Stream *stream) const {
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
  
  int unpack(Bakelite::Stream *stream) {
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
  char message[64];
  
  int pack(Bakelite::Stream *stream) const {
    int rcode = 0;
    rcode = write(stream, code);
    if(rcode != 0)
      return rcode;
    rcode = writeString(stream, message, 64);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
  
  int unpack(Bakelite::Stream *stream) {
    int rcode = 0;
    rcode = read(stream, code);
    if(rcode != 0)
      return rcode;
    rcode = readString(stream, message, 64);
    if(rcode != 0)
      return rcode;
    return rcode;
  }
};



template <class F = Bakelite::CobsFramer<Bakelite::Crc8, 73>>
class ProtocolBase {
public:
  using ReadFn  = int (*)();
  using WriteFn = size_t (*)(const char *data, size_t length);

  enum class Message {
    NoMessage = -1,
    TestMessage = 1,
    Ack = 2,
  };

  ProtocolBase(ReadFn read, WriteFn write): m_readFn(read), m_writeFn(write) {}

  Message poll() {
    int byte = (*m_readFn)();
    if(byte < 0) {
      return Message::NoMessage;
    }

    auto result = m_framer.readFrameByte((uint8_t)byte);
    if(result.status == Bakelite::CobsDecodeState::Decoded) {
      if(result.length == 0) {
        return Message::NoMessage;
      }
      
      m_receivedMessage = (Message)result.data[0];
      m_receivedFrameLength = result.length - 1;
      return m_receivedMessage;
    }

    return Message::NoMessage;
  }

  int send(const TestMessage &val) {
    Bakelite::BufferStream outStream((char *)m_framer.writeBuffer() + 1, m_framer.writeBufferSize() - 1);
    m_framer.writeBuffer()[0] = (uint8_t)Message::TestMessage;
    size_t startPos = outStream.pos();
    val.pack(&outStream);
    // Input fame size is the difference in stream position, plus the message byte
    size_t frameSize = ((outStream.pos() - startPos)) + 1;
    auto result = m_framer.encodeFrame(frameSize);

    if(result.status != 0) {
      return result.status;
    }
    
    int ret = (*m_writeFn)((const char *)result.data, result.length);
    return ret == result.length ? 0 : -1;
  }
  
  int send(const Ack &val) {
    Bakelite::BufferStream outStream((char *)m_framer.writeBuffer() + 1, m_framer.writeBufferSize() - 1);
    m_framer.writeBuffer()[0] = (uint8_t)Message::Ack;
    size_t startPos = outStream.pos();
    val.pack(&outStream);
    // Input fame size is the difference in stream position, plus the message byte
    size_t frameSize = ((outStream.pos() - startPos)) + 1;
    auto result = m_framer.encodeFrame(frameSize);

    if(result.status != 0) {
      return result.status;
    }
    
    int ret = (*m_writeFn)((const char *)result.data, result.length);
    return ret == result.length ? 0 : -1;
  }
  
  int decode(TestMessage &val, char *buffer = nullptr, size_t length = 0) {
    if(m_receivedMessage != Message::TestMessage) {
      return -1;
    }
    Bakelite::BufferStream stream(
      (char *)m_framer.readBuffer() + 1, m_receivedFrameLength,
      buffer, length
    );
    return val.unpack(&stream);
  }
  
  int decode(Ack &val, char *buffer = nullptr, size_t length = 0) {
    if(m_receivedMessage != Message::Ack) {
      return -1;
    }
    Bakelite::BufferStream stream(
      (char *)m_framer.readBuffer() + 1, m_receivedFrameLength,
      buffer, length
    );
    return val.unpack(&stream);
  }
  
private:
  ReadFn m_readFn;
  WriteFn m_writeFn;
  F m_framer;

  size_t m_receivedFrameLength = 0;
  Message m_receivedMessage = Message::NoMessage;
};

using Protocol = ProtocolBase<>;


