/*
 * C++ COBS Framer using common C99 implementation
 */

enum class CobsDecodeState {
  Decoded,
  NotReady,
  DecodeFailure,
  CrcFailure,
  BufferOverrun,
};

template <class C, size_t BufferSize>
class CobsFramer {
public:
  struct Result {
    int status;
    size_t length;
    char *data;
  };

  struct DecodeResult {
    CobsDecodeState status;
    size_t length;
    char *data;
  };

  // Single buffer access for zero-copy operations
  // Returns pointer to message area (after COBS overhead, at type byte position)
  char *buffer() {
    return m_buffer + messageOffset();
  }

  size_t bufferSize() {
    return BufferSize + 1;  // +1 for type byte
  }

  // Legacy API for compatibility
  char *readBuffer() {
    return m_buffer + messageOffset();
  }

  size_t readBufferSize() {
    return bufferSize();
  }

  char *writeBuffer() {
    return m_buffer + messageOffset();
  }

  size_t writeBufferSize() {
    return bufferSize();
  }

  Result encodeFrame(const char *data, size_t length) {
    char *msgStart = m_buffer + messageOffset();
    memcpy(msgStart, data, length);
    return encodeFrame(length);
  }

  Result encodeFrame(size_t length) {
    char *msgStart = m_buffer + messageOffset();

    if(C::size() > 0) {
      C crc;
      crc.update(msgStart, length);
      auto crc_val = crc.value();
      memcpy(msgStart + length, (void *)&crc_val, sizeof(crc_val));
    }

    auto result = bakelite_cobs_encode((void *)m_buffer, sizeof(m_buffer),
                              (void *)msgStart, length + C::size());
    if(result.status != 0) {
      return { 1, 0, nullptr };
    }

    m_buffer[result.out_len] = 0;

    return { 0, result.out_len + 1, m_buffer };
  }

  DecodeResult readFrameByte(char byte) {
    *m_readPos = byte;
    size_t length = (m_readPos - m_buffer) + 1;
    if(byte == 0) {
      m_readPos = m_buffer;
      return decodeFrame(length);
    }
    else if(length == sizeof(m_buffer)) {
      m_readPos = m_buffer;
      return { CobsDecodeState::BufferOverrun, 0, nullptr };
    }

    m_readPos++;
    return { CobsDecodeState::NotReady, 0, nullptr };
  }

private:
  DecodeResult decodeFrame(size_t length) {
    if(length == 1) {
      return { CobsDecodeState::DecodeFailure, 0, nullptr };
    }

    length--;  // Discard null byte

    // Decode in-place at buffer start
    auto result = bakelite_cobs_decode((void *)m_buffer, sizeof(m_buffer), (void *)m_buffer, length);
    if(result.status != 0) {
      return { CobsDecodeState::DecodeFailure, 0, nullptr };
    }

    // Length of decoded data without CRC
    length = result.out_len - C::size();

    if(C::size() > 0) {
      C crc;

      // Get the CRC from the end of the frame
      auto crc_val = crc.value();
      memcpy(&crc_val, m_buffer + length, sizeof(crc_val));

      crc.update(m_buffer, length);
      if(crc_val != crc.value()) {
        return { CobsDecodeState::CrcFailure, 0, nullptr };
      }
    }

    // Move decoded data to message offset position for consistent buffer layout
    size_t offset = messageOffset();
    if(offset > 0) {
      memmove(m_buffer + offset, m_buffer, length);
    }

    return { CobsDecodeState::Decoded, length, m_buffer + offset };
  }

  constexpr static size_t cobsOverhead(size_t bufferSize) {
    return (bufferSize + 253u) / 254u;
  }

  constexpr static size_t messageOffset() {
    return cobsOverhead(BufferSize + C::size());
  }

  constexpr static size_t totalBufferSize() {
    // COBS overhead + message + CRC + null terminator
    return messageOffset() + BufferSize + C::size() + 1;
  }

  char m_buffer[totalBufferSize()];
  char *m_readPos = m_buffer;
};
