/*
 * The code in this file is Copyright (c) Emma Powers 2021,
 * unless otherwise marked. This software is made available
 * under the terms of the MIT License, which can be found at the
 * bottom of this file.
*/

#ifndef __BAKELITE_H__
#define __BAKELITE_H__

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

namespace Bakelite {
  /*
  *
  *  Pre-Declarations
  *
  */
  // Pre-declarations of COBS functions
struct cobs_encode_result;
struct cobs_decode_result
{
    size_t              out_len;
    int                 status;
};
static cobs_encode_result cobs_encode(void *dst_buf_ptr, size_t dst_buf_len,
                                const void *src_ptr, size_t src_len);
static cobs_decode_result cobs_decode(void *dst_buf_ptr, size_t dst_buf_len,
                                const void *src_ptr, size_t src_len);

  /*
  *
  *  Serializer
  *
  */
  template <typename T, typename S = uint8_t>
struct SizedArray {
  T *data = nullptr;
  S size = 0;

  const T &at(size_t pos) const {
    return data[pos];
  }
};

class BufferStream {
public:
  BufferStream(char *buff, uint32_t size,
                char *heap = nullptr, uint32_t heapSize = 0):
    m_buff(buff),
    m_size(size),
    m_pos(0),
    m_heap(heap),
    m_heapPos(0),
    m_heapSize(heapSize)
  {}

  int write(const char *data, uint32_t length) {
    uint32_t endPos = m_pos + length;
    if(endPos > m_size) {
      return -1;
    }

    memcpy(m_buff+m_pos, data, length);
    m_pos += length;

    return 0;
  }

  int read(char *data, uint32_t length) {
    uint32_t endPos = m_pos + length;
    if(endPos > m_size) {
      return -2;
    }

    memcpy(data, m_buff+m_pos, length);
    m_pos += length;

    return 0;
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

  char *alloc(size_t bytes) {
    size_t newPos = m_heapPos + bytes;
    if(newPos >= m_heapSize)
      return nullptr;

    char *data = &m_heap[m_heapPos];
    m_heapPos = newPos;
    return data;
  }

private:
  char *m_buff;
  size_t m_size;
  size_t m_pos;
  
  char *m_heap;
  size_t m_heapPos;
  size_t m_heapSize;
};

template <class T, class V>
int write(T& stream, V val) {
  return stream.write((const char *)&val, sizeof(val));
}

template <class T, class V, class F>
int writeArray(T& stream, const V *val, int size, F writeCb) {
  for(int i = 0; i < size; i++) {
    int rcode = writeCb(stream, val[i]);
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class T, class V, class F>
int writeArray(T& stream, const SizedArray<V> &val, F writeCb) {
  write(stream, val.size);
  for(int i = 0; i < val.size; i++) {
    int rcode = writeCb(stream, val.at(i));
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class T>
int writeBytes(T& stream, const char *val, int size) {
  return stream.write((const char *)val, size);
}

template <class T>
int writeBytes(T& stream, const SizedArray<char> &val) {
  int rcode = write(stream, val.size);
  return stream.write((const char *)val.data, val.size);
}

template <class T>
int writeString(T& stream, const char *val, int size) {
  return stream.write(val, size);
}

template <class T>
int writeString(T& stream, const char *val) {
  if(val == nullptr) {
    return write(stream, (uint8_t)0);
  }

  uint8_t len = strlen(val);
  int rcode = stream.write(val, len);
  if(rcode != 0)
    return rcode;
  return write(stream, (uint8_t)0);
}

template <class T, class V>
int read(T& stream, V &val) {
  return stream.read((char *)&val, sizeof(val));
}

template <class T, class V, class F>
int readArray(T& stream, V val[], int size, F readCb) {
  for(int i = 0; i < size; i++) {
    int rcode = readCb(stream, val[i]);
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class T, class V, class F, class S = uint8_t>
int readArray(T& stream, SizedArray<V, S> &val, F readCb) {
  S size = 0;
  int rcode = read(stream, size);
  if(rcode != 0)
      return rcode;

  val.data = (V*)stream.alloc(sizeof(V) * size);
  val.size = size;

  if(val.data == nullptr) {
    return -4;
  }

  for(int i = 0; i < size; i++) {
    int rcode = readCb(stream, val.data[i]);
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class T>
int readBytes(T& stream, char *val, int size) {
  return stream.read(val, size);
}

template <class T, typename S = uint8_t>
int readBytes(T& stream, SizedArray<char, S> &val) {
  S size = 0;
  int rcode = read(stream, size);
  if(rcode != 0)
      return rcode;

  val.data = stream.alloc(size);
  val.size = size;

  if(val.data == nullptr) {
    return -5;
  }

  return stream.read(val.data, val.size);
}

template <class T>
int readString(T& stream, char *val, int size) {
  return stream.read(val, size);
}

template <class T>
int readString(T& stream, char* &val) {
  char *newByte = stream.alloc(1);
  val = newByte;

  do {
    int rcode = stream.read(newByte, 1);
    if(rcode != 0)
      return rcode;

    if(*newByte == 0) {
      return 0;
    }
  } while((newByte = stream.alloc(1)) != nullptr);

  return -6;
}

  /*
  *
  *  CRC
  *
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

template <typename CrcFunc, typename CrcType>
class Crc {
public:
  constexpr static size_t size() {
    return sizeof(CrcType);
  }

  CrcType value() const {
    return m_lastVal;
  }

  void update(const char *data, size_t length) {
    CrcFunc fn;
    m_lastVal = fn(data, length, m_lastVal);
  }
private:
  CrcType m_lastVal = 0;
};

struct crc8_fn;
struct crc16_fn;
struct crc32_fn;


using Crc8 = Crc<crc8_fn, uint8_t>;
using Crc16 = Crc<crc16_fn, uint16_t>;
using Crc32 = Crc<crc32_fn, uint32_t>;

/*
  *  Auto generated CRC functions
  */

// The CRC lookup tables are stored as const variables. On many platforms,
// const variables are stored in flash memroy. On AVR though, they are
// loaded into RAM on startup. A special PROGMEM macro is available on AVRs
// to indicate constants should be stored in program memory (flash).
// So if this macro is available, use it and assume we're on an AVR. 
#ifdef PROGMEM
  #define BAKELITE_CONST PROGMEM
  // PROGMEM variables need to be accessed using pgm_read_* functions.
  #define BAKELITE_CONST_8(x)  pgm_read_byte(&(x))
  #define BAKELITE_CONST_16(x) pgm_read_dword(&(x))
  #define BAKELITE_CONST_32(x) ((uint32_t)pgm_read_dword((char *)&(x) + 2) << 16 | pgm_read_dword(&(x)))
#else
  #define BAKELITE_CONST
  #define BAKELITE_CONST_8(x)  x
  #define BAKELITE_CONST_16(x) x
  #define BAKELITE_CONST_32(x) x
#endif

// Automatically generated CRC function
// polynomial: 0x107
struct crc8_fn {
  uint8_t operator()(const char *data, int len, uint8_t crc) {
    const unsigned char *uData = (unsigned char *)data;

    static const uint8_t table[256] BAKELITE_CONST = {
    0x00U,0x07U,0x0EU,0x09U,0x1CU,0x1BU,0x12U,0x15U,
    0x38U,0x3FU,0x36U,0x31U,0x24U,0x23U,0x2AU,0x2DU,
    0x70U,0x77U,0x7EU,0x79U,0x6CU,0x6BU,0x62U,0x65U,
    0x48U,0x4FU,0x46U,0x41U,0x54U,0x53U,0x5AU,0x5DU,
    0xE0U,0xE7U,0xEEU,0xE9U,0xFCU,0xFBU,0xF2U,0xF5U,
    0xD8U,0xDFU,0xD6U,0xD1U,0xC4U,0xC3U,0xCAU,0xCDU,
    0x90U,0x97U,0x9EU,0x99U,0x8CU,0x8BU,0x82U,0x85U,
    0xA8U,0xAFU,0xA6U,0xA1U,0xB4U,0xB3U,0xBAU,0xBDU,
    0xC7U,0xC0U,0xC9U,0xCEU,0xDBU,0xDCU,0xD5U,0xD2U,
    0xFFU,0xF8U,0xF1U,0xF6U,0xE3U,0xE4U,0xEDU,0xEAU,
    0xB7U,0xB0U,0xB9U,0xBEU,0xABU,0xACU,0xA5U,0xA2U,
    0x8FU,0x88U,0x81U,0x86U,0x93U,0x94U,0x9DU,0x9AU,
    0x27U,0x20U,0x29U,0x2EU,0x3BU,0x3CU,0x35U,0x32U,
    0x1FU,0x18U,0x11U,0x16U,0x03U,0x04U,0x0DU,0x0AU,
    0x57U,0x50U,0x59U,0x5EU,0x4BU,0x4CU,0x45U,0x42U,
    0x6FU,0x68U,0x61U,0x66U,0x73U,0x74U,0x7DU,0x7AU,
    0x89U,0x8EU,0x87U,0x80U,0x95U,0x92U,0x9BU,0x9CU,
    0xB1U,0xB6U,0xBFU,0xB8U,0xADU,0xAAU,0xA3U,0xA4U,
    0xF9U,0xFEU,0xF7U,0xF0U,0xE5U,0xE2U,0xEBU,0xECU,
    0xC1U,0xC6U,0xCFU,0xC8U,0xDDU,0xDAU,0xD3U,0xD4U,
    0x69U,0x6EU,0x67U,0x60U,0x75U,0x72U,0x7BU,0x7CU,
    0x51U,0x56U,0x5FU,0x58U,0x4DU,0x4AU,0x43U,0x44U,
    0x19U,0x1EU,0x17U,0x10U,0x05U,0x02U,0x0BU,0x0CU,
    0x21U,0x26U,0x2FU,0x28U,0x3DU,0x3AU,0x33U,0x34U,
    0x4EU,0x49U,0x40U,0x47U,0x52U,0x55U,0x5CU,0x5BU,
    0x76U,0x71U,0x78U,0x7FU,0x6AU,0x6DU,0x64U,0x63U,
    0x3EU,0x39U,0x30U,0x37U,0x22U,0x25U,0x2CU,0x2BU,
    0x06U,0x01U,0x08U,0x0FU,0x1AU,0x1DU,0x14U,0x13U,
    0xAEU,0xA9U,0xA0U,0xA7U,0xB2U,0xB5U,0xBCU,0xBBU,
    0x96U,0x91U,0x98U,0x9FU,0x8AU,0x8DU,0x84U,0x83U,
    0xDEU,0xD9U,0xD0U,0xD7U,0xC2U,0xC5U,0xCCU,0xCBU,
    0xE6U,0xE1U,0xE8U,0xEFU,0xFAU,0xFDU,0xF4U,0xF3U,
    };

    while (len > 0)
    {
      crc = BAKELITE_CONST_8(table[*uData ^ (uint8_t)crc]);
      uData++;
      len--;
    }
    return crc;
  }
};

// Automatically generated CRC function
// polynomial: 0x18005, bit reverse algorithm
struct crc16_fn {
  uint16_t operator()(const char *data, int len, uint16_t crc) {
    const unsigned char *uData = (unsigned char *)data;

    static const uint16_t table[256] BAKELITE_CONST = {
    0x0000U,0xC0C1U,0xC181U,0x0140U,0xC301U,0x03C0U,0x0280U,0xC241U,
    0xC601U,0x06C0U,0x0780U,0xC741U,0x0500U,0xC5C1U,0xC481U,0x0440U,
    0xCC01U,0x0CC0U,0x0D80U,0xCD41U,0x0F00U,0xCFC1U,0xCE81U,0x0E40U,
    0x0A00U,0xCAC1U,0xCB81U,0x0B40U,0xC901U,0x09C0U,0x0880U,0xC841U,
    0xD801U,0x18C0U,0x1980U,0xD941U,0x1B00U,0xDBC1U,0xDA81U,0x1A40U,
    0x1E00U,0xDEC1U,0xDF81U,0x1F40U,0xDD01U,0x1DC0U,0x1C80U,0xDC41U,
    0x1400U,0xD4C1U,0xD581U,0x1540U,0xD701U,0x17C0U,0x1680U,0xD641U,
    0xD201U,0x12C0U,0x1380U,0xD341U,0x1100U,0xD1C1U,0xD081U,0x1040U,
    0xF001U,0x30C0U,0x3180U,0xF141U,0x3300U,0xF3C1U,0xF281U,0x3240U,
    0x3600U,0xF6C1U,0xF781U,0x3740U,0xF501U,0x35C0U,0x3480U,0xF441U,
    0x3C00U,0xFCC1U,0xFD81U,0x3D40U,0xFF01U,0x3FC0U,0x3E80U,0xFE41U,
    0xFA01U,0x3AC0U,0x3B80U,0xFB41U,0x3900U,0xF9C1U,0xF881U,0x3840U,
    0x2800U,0xE8C1U,0xE981U,0x2940U,0xEB01U,0x2BC0U,0x2A80U,0xEA41U,
    0xEE01U,0x2EC0U,0x2F80U,0xEF41U,0x2D00U,0xEDC1U,0xEC81U,0x2C40U,
    0xE401U,0x24C0U,0x2580U,0xE541U,0x2700U,0xE7C1U,0xE681U,0x2640U,
    0x2200U,0xE2C1U,0xE381U,0x2340U,0xE101U,0x21C0U,0x2080U,0xE041U,
    0xA001U,0x60C0U,0x6180U,0xA141U,0x6300U,0xA3C1U,0xA281U,0x6240U,
    0x6600U,0xA6C1U,0xA781U,0x6740U,0xA501U,0x65C0U,0x6480U,0xA441U,
    0x6C00U,0xACC1U,0xAD81U,0x6D40U,0xAF01U,0x6FC0U,0x6E80U,0xAE41U,
    0xAA01U,0x6AC0U,0x6B80U,0xAB41U,0x6900U,0xA9C1U,0xA881U,0x6840U,
    0x7800U,0xB8C1U,0xB981U,0x7940U,0xBB01U,0x7BC0U,0x7A80U,0xBA41U,
    0xBE01U,0x7EC0U,0x7F80U,0xBF41U,0x7D00U,0xBDC1U,0xBC81U,0x7C40U,
    0xB401U,0x74C0U,0x7580U,0xB541U,0x7700U,0xB7C1U,0xB681U,0x7640U,
    0x7200U,0xB2C1U,0xB381U,0x7340U,0xB101U,0x71C0U,0x7080U,0xB041U,
    0x5000U,0x90C1U,0x9181U,0x5140U,0x9301U,0x53C0U,0x5280U,0x9241U,
    0x9601U,0x56C0U,0x5780U,0x9741U,0x5500U,0x95C1U,0x9481U,0x5440U,
    0x9C01U,0x5CC0U,0x5D80U,0x9D41U,0x5F00U,0x9FC1U,0x9E81U,0x5E40U,
    0x5A00U,0x9AC1U,0x9B81U,0x5B40U,0x9901U,0x59C0U,0x5880U,0x9841U,
    0x8801U,0x48C0U,0x4980U,0x8941U,0x4B00U,0x8BC1U,0x8A81U,0x4A40U,
    0x4E00U,0x8EC1U,0x8F81U,0x4F40U,0x8D01U,0x4DC0U,0x4C80U,0x8C41U,
    0x4400U,0x84C1U,0x8581U,0x4540U,0x8701U,0x47C0U,0x4680U,0x8641U,
    0x8201U,0x42C0U,0x4380U,0x8341U,0x4100U,0x81C1U,0x8081U,0x4040U,
    };

    while (len > 0)
    {
      crc = BAKELITE_CONST_16(table[*uData ^ (uint8_t)crc]) ^ (crc >> 8);
      uData++;
      len--;
    }
    return crc;
  }
};

// Automatically generated CRC function
// polynomial: 0x104C11DB7, bit reverse algorithm
struct crc32_fn {
  uint32_t operator()(const char *data, int len, uint32_t crc) {
    const unsigned char *uData = (unsigned char *)data;

    static const uint32_t table[256] BAKELITE_CONST = {
    0x00000000U,0x77073096U,0xEE0E612CU,0x990951BAU,
    0x076DC419U,0x706AF48FU,0xE963A535U,0x9E6495A3U,
    0x0EDB8832U,0x79DCB8A4U,0xE0D5E91EU,0x97D2D988U,
    0x09B64C2BU,0x7EB17CBDU,0xE7B82D07U,0x90BF1D91U,
    0x1DB71064U,0x6AB020F2U,0xF3B97148U,0x84BE41DEU,
    0x1ADAD47DU,0x6DDDE4EBU,0xF4D4B551U,0x83D385C7U,
    0x136C9856U,0x646BA8C0U,0xFD62F97AU,0x8A65C9ECU,
    0x14015C4FU,0x63066CD9U,0xFA0F3D63U,0x8D080DF5U,
    0x3B6E20C8U,0x4C69105EU,0xD56041E4U,0xA2677172U,
    0x3C03E4D1U,0x4B04D447U,0xD20D85FDU,0xA50AB56BU,
    0x35B5A8FAU,0x42B2986CU,0xDBBBC9D6U,0xACBCF940U,
    0x32D86CE3U,0x45DF5C75U,0xDCD60DCFU,0xABD13D59U,
    0x26D930ACU,0x51DE003AU,0xC8D75180U,0xBFD06116U,
    0x21B4F4B5U,0x56B3C423U,0xCFBA9599U,0xB8BDA50FU,
    0x2802B89EU,0x5F058808U,0xC60CD9B2U,0xB10BE924U,
    0x2F6F7C87U,0x58684C11U,0xC1611DABU,0xB6662D3DU,
    0x76DC4190U,0x01DB7106U,0x98D220BCU,0xEFD5102AU,
    0x71B18589U,0x06B6B51FU,0x9FBFE4A5U,0xE8B8D433U,
    0x7807C9A2U,0x0F00F934U,0x9609A88EU,0xE10E9818U,
    0x7F6A0DBBU,0x086D3D2DU,0x91646C97U,0xE6635C01U,
    0x6B6B51F4U,0x1C6C6162U,0x856530D8U,0xF262004EU,
    0x6C0695EDU,0x1B01A57BU,0x8208F4C1U,0xF50FC457U,
    0x65B0D9C6U,0x12B7E950U,0x8BBEB8EAU,0xFCB9887CU,
    0x62DD1DDFU,0x15DA2D49U,0x8CD37CF3U,0xFBD44C65U,
    0x4DB26158U,0x3AB551CEU,0xA3BC0074U,0xD4BB30E2U,
    0x4ADFA541U,0x3DD895D7U,0xA4D1C46DU,0xD3D6F4FBU,
    0x4369E96AU,0x346ED9FCU,0xAD678846U,0xDA60B8D0U,
    0x44042D73U,0x33031DE5U,0xAA0A4C5FU,0xDD0D7CC9U,
    0x5005713CU,0x270241AAU,0xBE0B1010U,0xC90C2086U,
    0x5768B525U,0x206F85B3U,0xB966D409U,0xCE61E49FU,
    0x5EDEF90EU,0x29D9C998U,0xB0D09822U,0xC7D7A8B4U,
    0x59B33D17U,0x2EB40D81U,0xB7BD5C3BU,0xC0BA6CADU,
    0xEDB88320U,0x9ABFB3B6U,0x03B6E20CU,0x74B1D29AU,
    0xEAD54739U,0x9DD277AFU,0x04DB2615U,0x73DC1683U,
    0xE3630B12U,0x94643B84U,0x0D6D6A3EU,0x7A6A5AA8U,
    0xE40ECF0BU,0x9309FF9DU,0x0A00AE27U,0x7D079EB1U,
    0xF00F9344U,0x8708A3D2U,0x1E01F268U,0x6906C2FEU,
    0xF762575DU,0x806567CBU,0x196C3671U,0x6E6B06E7U,
    0xFED41B76U,0x89D32BE0U,0x10DA7A5AU,0x67DD4ACCU,
    0xF9B9DF6FU,0x8EBEEFF9U,0x17B7BE43U,0x60B08ED5U,
    0xD6D6A3E8U,0xA1D1937EU,0x38D8C2C4U,0x4FDFF252U,
    0xD1BB67F1U,0xA6BC5767U,0x3FB506DDU,0x48B2364BU,
    0xD80D2BDAU,0xAF0A1B4CU,0x36034AF6U,0x41047A60U,
    0xDF60EFC3U,0xA867DF55U,0x316E8EEFU,0x4669BE79U,
    0xCB61B38CU,0xBC66831AU,0x256FD2A0U,0x5268E236U,
    0xCC0C7795U,0xBB0B4703U,0x220216B9U,0x5505262FU,
    0xC5BA3BBEU,0xB2BD0B28U,0x2BB45A92U,0x5CB36A04U,
    0xC2D7FFA7U,0xB5D0CF31U,0x2CD99E8BU,0x5BDEAE1DU,
    0x9B64C2B0U,0xEC63F226U,0x756AA39CU,0x026D930AU,
    0x9C0906A9U,0xEB0E363FU,0x72076785U,0x05005713U,
    0x95BF4A82U,0xE2B87A14U,0x7BB12BAEU,0x0CB61B38U,
    0x92D28E9BU,0xE5D5BE0DU,0x7CDCEFB7U,0x0BDBDF21U,
    0x86D3D2D4U,0xF1D4E242U,0x68DDB3F8U,0x1FDA836EU,
    0x81BE16CDU,0xF6B9265BU,0x6FB077E1U,0x18B74777U,
    0x88085AE6U,0xFF0F6A70U,0x66063BCAU,0x11010B5CU,
    0x8F659EFFU,0xF862AE69U,0x616BFFD3U,0x166CCF45U,
    0xA00AE278U,0xD70DD2EEU,0x4E048354U,0x3903B3C2U,
    0xA7672661U,0xD06016F7U,0x4969474DU,0x3E6E77DBU,
    0xAED16A4AU,0xD9D65ADCU,0x40DF0B66U,0x37D83BF0U,
    0xA9BCAE53U,0xDEBB9EC5U,0x47B2CF7FU,0x30B5FFE9U,
    0xBDBDF21CU,0xCABAC28AU,0x53B39330U,0x24B4A3A6U,
    0xBAD03605U,0xCDD70693U,0x54DE5729U,0x23D967BFU,
    0xB3667A2EU,0xC4614AB8U,0x5D681B02U,0x2A6F2B94U,
    0xB40BBE37U,0xC30C8EA1U,0x5A05DF1BU,0x2D02EF8DU,
    };

    crc = crc ^ 0xFFFFFFFFU;
    while (len > 0)
    {
      crc = BAKELITE_CONST_32(table[*uData ^ (uint8_t)crc]) ^ (crc >> 8);
      uData++;
      len--;
    }
    crc = crc ^ 0xFFFFFFFFU;
    return crc;
  }
};

  /*
  *
  *  COBS Framer
  *
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

  char *readBuffer() {
    return m_readBuffer;
  }

  size_t readBufferSize() {
    return sizeof(m_readBuffer);
  }

  char *writeBuffer() {
    return m_writePtr;
  }

  size_t writeBufferSize() {
    return sizeof(m_writeBuffer) - overhead(BufferSize);
  }

  Result encodeFrame(const char *data, size_t length) {
    assert(data);
    assert(length <= BufferSize);

    memcpy(m_writePtr, data, length);
    return encodeFrame(length);
  }
  
  Result encodeFrame(size_t length) {
    assert(length <= BufferSize);

    if(C::size() > 0) {
      C crc;
      crc.update(m_writePtr, length);
      auto crc_val = crc.value();
      memcpy(m_writePtr + length, (void *)&crc_val, sizeof(crc_val));
    }

    auto result = cobs_encode((void *)m_writeBuffer, sizeof(m_writeBuffer),
                              (void *)m_writePtr, length+C::size());
    if(result.status != 0) {
      return { 1, 0, nullptr };
    }

    m_writeBuffer[result.out_len] = 0;

    return { 0, result.out_len + 1, m_writeBuffer };
  }

  DecodeResult readFrameByte(char byte) {
    *m_readPos = byte;
    size_t length = (m_readPos - m_readBuffer) + 1;
    if(byte == 0) {
      m_readPos = m_readBuffer;
      return decodeFrame(length);
    }
    else if(length == sizeof(m_readBuffer)) {
      m_readPos = m_readBuffer;
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

    length--; // Discard null byte

    auto result = cobs_decode((void *)m_readBuffer, sizeof(m_readBuffer), (void *)m_readBuffer, length);
    if(result.status != 0) {
      return { CobsDecodeState::DecodeFailure, 0, nullptr };
    }

    // length of the decoded data without CRC
    length = result.out_len - C::size();

    if(C::size() > 0) {
      C crc;

      // Get the CRC from the end of the frame
      auto crc_val = crc.value();
      memcpy(&crc_val, m_readBuffer + length, sizeof(crc_val));

      crc.update(m_readBuffer, length);
      if(crc_val != crc.value()) {
        return { CobsDecodeState::CrcFailure, 0, nullptr };
      }
    }

    return { CobsDecodeState::Decoded, length, m_readBuffer };
  }

  constexpr static size_t cobsOverhead(size_t bufferSize) {
    return (bufferSize + 253u)/254u;
  }
  constexpr static size_t overhead(size_t bufferSize) {
    return cobsOverhead(BufferSize + C::size()) + C::size() + 1;
  }

  char m_readBuffer[BufferSize + overhead(BufferSize)];
  char *m_readPos = m_readBuffer;
  char m_writeBuffer[BufferSize + overhead(BufferSize)];
  char *m_writePtr = m_writeBuffer + cobsOverhead(BufferSize);
};

/***************
 * The below COBS function are Copyright (c) 2010 Craig McQueen
 * And licensed under the MIT license, which can be found at the end of this file.
 * 
 * Source: https://github.com/cmcqueen/cobs-c
 * Commit: f4b812953e19bcece1a994d33f370652dba2bf1b 
 ***************/ 

#define COBS_ENCODE_DST_BUF_LEN_MAX(SRC_LEN)            ((SRC_LEN) + (((SRC_LEN) + 253u)/254u))
#define COBS_DECODE_DST_BUF_LEN_MAX(SRC_LEN)            (((SRC_LEN) == 0) ? 0u : ((SRC_LEN) - 1u))
#define COBS_ENCODE_SRC_OFFSET(SRC_LEN)                 (((SRC_LEN) + 253u)/254u)

typedef enum
{
    COBS_ENCODE_OK                  = 0x00,
    COBS_ENCODE_NULL_POINTER        = 0x01,
    COBS_ENCODE_OUT_BUFFER_OVERFLOW = 0x02
} cobs_encode_status;

struct cobs_encode_result
{
    size_t              out_len;
    int                 status;
};

typedef enum
{
    COBS_DECODE_OK                  = 0x00,
    COBS_DECODE_NULL_POINTER        = 0x01,
    COBS_DECODE_OUT_BUFFER_OVERFLOW = 0x02,
    COBS_DECODE_ZERO_BYTE_IN_INPUT  = 0x04,
    COBS_DECODE_INPUT_TOO_SHORT     = 0x08
} cobs_decode_status;

/* COBS-encode a string of input bytes.
*
* dst_buf_ptr:    The buffer into which the result will be written
* dst_buf_len:    Length of the buffer into which the result will be written
* src_ptr:        The byte string to be encoded
* src_len         Length of the byte string to be encoded
*
* returns:        A struct containing the success status of the encoding
*                 operation and the length of the result (that was written to
*                 dst_buf_ptr)
*/
static cobs_encode_result cobs_encode(void *dst_buf_ptr, size_t dst_buf_len,
                                const void *src_ptr, size_t src_len)
{
  cobs_encode_result result = {0, COBS_ENCODE_OK};
  const uint8_t *src_read_ptr = (uint8_t *)src_ptr;
  const uint8_t *src_end_ptr = (uint8_t *)src_read_ptr + src_len;
  uint8_t *dst_buf_start_ptr = (uint8_t *)dst_buf_ptr;
  uint8_t *dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
  uint8_t *dst_code_write_ptr = (uint8_t *)dst_buf_ptr;
  uint8_t *dst_write_ptr = dst_code_write_ptr + 1;
  uint8_t src_byte = 0;
  uint8_t search_len = 1;

  /* First, do a NULL pointer check and return immediately if it fails. */
  if ((dst_buf_ptr == NULL) || (src_ptr == NULL))
  {
    result.status = COBS_ENCODE_NULL_POINTER;
    return result;
  }

  if (src_len != 0)
  {
    /* Iterate over the source bytes */
    for (;;)
    {
      /* Check for running out of output buffer space */
      if (dst_write_ptr >= dst_buf_end_ptr)
      {
        result.status |= COBS_ENCODE_OUT_BUFFER_OVERFLOW;
        break;
      }

      src_byte = *src_read_ptr++;
      if (src_byte == 0)
      {
        /* We found a zero byte */
        *dst_code_write_ptr = search_len;
        dst_code_write_ptr = dst_write_ptr++;
        search_len = 1;
        if (src_read_ptr >= src_end_ptr)
        {
          break;
        }
      }
      else
      {
        /* Copy the non-zero byte to the destination buffer */
        *dst_write_ptr++ = src_byte;
        search_len++;
        if (src_read_ptr >= src_end_ptr)
        {
          break;
        }
        if (search_len == 0xFF)
        {
          /* We have a long string of non-zero bytes, so we need
                    * to write out a length code of 0xFF. */
          *dst_code_write_ptr = search_len;
          dst_code_write_ptr = dst_write_ptr++;
          search_len = 1;
        }
      }
    }
  }

  /* We've reached the end of the source data (or possibly run out of output buffer)
    * Finalise the remaining output. In particular, write the code (length) byte.
    * Update the pointer to calculate the final output length.
    */
  if (dst_code_write_ptr >= dst_buf_end_ptr)
  {
    /* We've run out of output buffer to write the code byte. */
    result.status |= COBS_ENCODE_OUT_BUFFER_OVERFLOW;
    dst_write_ptr = dst_buf_end_ptr;
  }
  else
  {
    /* Write the last code (length) byte. */
    *dst_code_write_ptr = search_len;
  }

  /* Calculate the output length, from the value of dst_code_write_ptr */
  result.out_len = dst_write_ptr - dst_buf_start_ptr;

  return result;
}

/* Decode a COBS byte string.
*
* dst_buf_ptr:    The buffer into which the result will be written
* dst_buf_len:    Length of the buffer into which the result will be written
* src_ptr:        The byte string to be decoded
* src_len         Length of the byte string to be decoded
*
* returns:        A struct containing the success status of the decoding
*                 operation and the length of the result (that was written to
*                 dst_buf_ptr)
*/
static cobs_decode_result cobs_decode(void *dst_buf_ptr, size_t dst_buf_len,
                                const void *src_ptr, size_t src_len)
{
  cobs_decode_result result = {0, COBS_DECODE_OK};
  const uint8_t *src_read_ptr = (uint8_t *)src_ptr;
  const uint8_t *src_end_ptr = (uint8_t *)src_read_ptr + src_len;
  uint8_t *dst_buf_start_ptr = (uint8_t *)dst_buf_ptr;
  uint8_t *dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
  uint8_t *dst_write_ptr = (uint8_t *)dst_buf_ptr;
  size_t remaining_bytes;
  uint8_t src_byte;
  uint8_t i;
  uint8_t len_code;

  /* First, do a NULL pointer check and return immediately if it fails. */
  if ((dst_buf_ptr == NULL) || (src_ptr == NULL))
  {
    result.status = COBS_DECODE_NULL_POINTER;
    return result;
  }

  if (src_len != 0)
  {
    for (;;)
    {
      len_code = *src_read_ptr++;
      if (len_code == 0)
      {
        result.status |= COBS_DECODE_ZERO_BYTE_IN_INPUT;
        break;
      }
      len_code--;

      /* Check length code against remaining input bytes */
      remaining_bytes = src_end_ptr - src_read_ptr;
      if (len_code > remaining_bytes)
      {
        result.status |= COBS_DECODE_INPUT_TOO_SHORT;
        len_code = remaining_bytes;
      }

      /* Check length code against remaining output buffer space */
      remaining_bytes = dst_buf_end_ptr - dst_write_ptr;
      if (len_code > remaining_bytes)
      {
        result.status |= COBS_DECODE_OUT_BUFFER_OVERFLOW;
        len_code = remaining_bytes;
      }

      for (i = len_code; i != 0; i--)
      {
        src_byte = *src_read_ptr++;
        if (src_byte == 0)
        {
          result.status |= COBS_DECODE_ZERO_BYTE_IN_INPUT;
        }
        *dst_write_ptr++ = src_byte;
      }

      if (src_read_ptr >= src_end_ptr)
      {
        break;
      }

      /* Add a zero to the end */
      if (len_code != 0xFE)
      {
        if (dst_write_ptr >= dst_buf_end_ptr)
        {
          result.status |= COBS_DECODE_OUT_BUFFER_OVERFLOW;
          break;
        }
        *dst_write_ptr++ = 0;
      }
    }
  }

  result.out_len = dst_write_ptr - dst_buf_start_ptr;

  return result;
}
}

/* 
The MIT License
---------------

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. 
*/

#endif // __BAKELITE_H__
