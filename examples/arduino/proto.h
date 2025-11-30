#pragma once

#include "bakelite.h"

// Platform check for packed struct support (unaligned access required)
#if defined(__AVR__) || (defined(__ARM_ARCH) && __ARM_ARCH >= 7) || \
    defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
  #define BAKELITE_UNALIGNED_OK 1
#else
  #define BAKELITE_UNALIGNED_OK 0
#endif

static_assert(BAKELITE_UNALIGNED_OK,
  "This code requires unaligned memory access. Regenerate with --unpacked for "
  "Cortex-M0, RISC-V, ESP32, PIC32, or other platforms without unaligned access support.");
struct __attribute__((packed)) TestMessage {
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



struct __attribute__((packed)) Ack {
  uint8_t code;
  char message[64];
  
  template<class T>
  int pack(T &stream) const {
    int rcode = 0;
    rcode = write(stream, code);
    if(rcode != 0)
      return rcode;
    rcode = writeString(stream, message, 64);
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

    auto result = m_framer.readFrameByte((char)byte);
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

  // Zero-copy message access - returns reference to message in buffer
  template<typename T>
  T& message() {
    return *reinterpret_cast<T*>(m_framer.buffer() + 1);
  }

  template<typename T>
  const T& message() const {
    return *reinterpret_cast<const T*>(m_framer.buffer() + 1);
  }

  // Zero-copy send overloads for each message type
  int send(const TestMessage*) {
    m_framer.buffer()[0] = static_cast<char>(Message::TestMessage);
    size_t frameSize = sizeof(TestMessage) + 1;
    auto result = m_framer.encodeFrame(frameSize);

    if(result.status != 0) {
      return result.status;
    }

    size_t ret = (*m_writeFn)(result.data, result.length);
    return ret == result.length ? 0 : -1;
  }
  
  int send(const Ack*) {
    m_framer.buffer()[0] = static_cast<char>(Message::Ack);
    size_t frameSize = sizeof(Ack) + 1;
    auto result = m_framer.encodeFrame(frameSize);

    if(result.status != 0) {
      return result.status;
    }

    size_t ret = (*m_writeFn)(result.data, result.length);
    return ret == result.length ? 0 : -1;
  }
  
  // Zero-copy send helper - use as: send<MsgType>()
  template<typename T>
  int send() {
    return send(static_cast<const T*>(nullptr));
  }
  // Copy-based send (works with variable-length fields, compatible with both modes)
  int send(const TestMessage &val) {
    Bakelite::BufferStream outStream(m_framer.buffer() + 1, m_framer.bufferSize() - 1);
    m_framer.buffer()[0] = static_cast<char>(Message::TestMessage);
    size_t startPos = outStream.pos();
    val.pack(outStream);
    size_t frameSize = (outStream.pos() - startPos) + 1;
    auto result = m_framer.encodeFrame(frameSize);

    if(result.status != 0) {
      return result.status;
    }

    size_t ret = (*m_writeFn)(result.data, result.length);
    return ret == result.length ? 0 : -1;
  }
  
  int send(const Ack &val) {
    Bakelite::BufferStream outStream(m_framer.buffer() + 1, m_framer.bufferSize() - 1);
    m_framer.buffer()[0] = static_cast<char>(Message::Ack);
    size_t startPos = outStream.pos();
    val.pack(outStream);
    size_t frameSize = (outStream.pos() - startPos) + 1;
    auto result = m_framer.encodeFrame(frameSize);

    if(result.status != 0) {
      return result.status;
    }

    size_t ret = (*m_writeFn)(result.data, result.length);
    return ret == result.length ? 0 : -1;
  }
  
  // Copy-based decode (works with variable-length fields, compatible with both modes)
  int decode(TestMessage &val, char *buffer = nullptr, size_t length = 0) {
    if(m_receivedMessage != Message::TestMessage) {
      return -1;
    }
    Bakelite::BufferStream stream(
      m_framer.buffer() + 1, m_receivedFrameLength,
      buffer, length
    );
    return val.unpack(stream);
  }
  
  int decode(Ack &val, char *buffer = nullptr, size_t length = 0) {
    if(m_receivedMessage != Message::Ack) {
      return -1;
    }
    Bakelite::BufferStream stream(
      m_framer.buffer() + 1, m_receivedFrameLength,
      buffer, length
    );
    return val.unpack(stream);
  }
  
private:
  ReadFn m_readFn;
  WriteFn m_writeFn;
  F m_framer;

  size_t m_receivedFrameLength = 0;
  Message m_receivedMessage = Message::NoMessage;
};

using Protocol = ProtocolBase<>;


