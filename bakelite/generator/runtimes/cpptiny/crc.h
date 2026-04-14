/*
 * C++ CRC wrapper classes using common C99 implementation
 */

class CrcNoop {
public:
  constexpr static size_t size() {
    return 0;
  }

  int value() const {
    return 0;
  }

  void update(const char *c, size_t length) {
  }
};

template <typename CrcType>
class Crc8Impl {
public:
  constexpr static size_t size() {
    return sizeof(CrcType);
  }

  CrcType value() const {
    return m_lastVal;
  }

  void update(const char *data, size_t length) {
    m_lastVal = bakelite_crc8((const uint8_t *)data, length, m_lastVal);
  }
private:
  CrcType m_lastVal = 0;
};

template <typename CrcType>
class Crc16Impl {
public:
  constexpr static size_t size() {
    return sizeof(CrcType);
  }

  CrcType value() const {
    return m_lastVal;
  }

  void update(const char *data, size_t length) {
    m_lastVal = bakelite_crc16((const uint8_t *)data, length, m_lastVal);
  }
private:
  CrcType m_lastVal = 0;
};

template <typename CrcType>
class Crc32Impl {
public:
  constexpr static size_t size() {
    return sizeof(CrcType);
  }

  CrcType value() const {
    return m_lastVal;
  }

  void update(const char *data, size_t length) {
    m_lastVal = bakelite_crc32((const uint8_t *)data, length, m_lastVal);
  }
private:
  CrcType m_lastVal = 0;
};

using Crc8 = Crc8Impl<uint8_t>;
using Crc16 = Crc16Impl<uint16_t>;
using Crc32 = Crc32Impl<uint32_t>;
