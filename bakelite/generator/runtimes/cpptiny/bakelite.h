/*
 * The code in this file is Copyright (c) Emma Powers 2021,
 * unless otherwise marked. This software is made available
 * under the terms of the MIT License, which can be found at the
 * bottom of this file.
*/

#ifndef __BAKELITE_H__
#define __BAKELITE_H__

#include <cstdint>
#include <limits>
#include <string.h>
#include <stdio.h>

namespace Bakelite {
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
      if(endPos >= m_size) {
        return -1;
      }

      memcpy(m_buff+m_pos, data, length);
      m_pos += length;

      return 0;
    }

    int read(char *data, uint32_t length) {
      uint32_t endPos = m_pos + length;
      if(endPos >= m_size) {
        return -1;
      }

      memcpy(data, m_buff+m_pos, length);
      m_pos += length;

      return 0;
    }

    int seek(uint32_t pos) {
      if(pos >= m_size) {
        return -1;
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
  int writeBytes(T& stream, const uint8_t *val, int size) {
    return stream.write((const char *)val, size);
  }

  template <class T>
  int writeBytes(T& stream, const SizedArray<uint8_t> &val) {
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
      return -1;
    }

    for(int i = 0; i < size; i++) {
      int rcode = readCb(stream, val.data[i]);
      if(rcode != 0)
        return rcode;
    }
    return 0;
  }

  template <class T>
  int readBytes(T& stream, uint8_t *val, int size) {
    return stream.read((char *)val, size);
  }

  template <class T, typename S = uint8_t>
  int readBytes(T& stream, SizedArray<uint8_t, S> &val) {
    S size = 0;
    int rcode = read(stream, size);
    if(rcode != 0)
        return rcode;

    val.data = (uint8_t*)stream.alloc(size);
    val.size = size;

    if(val.data == nullptr) {
      return -1;
    }

    return stream.read((char *)val.data, val.size);
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

    return -1;
  }

  // Pre-declarations of COBS functions
  struct cobs_encode_result;
  struct cobs_decode_result
  {
      size_t              out_len;
      int                 status;
  };
  cobs_encode_result cobs_encode(uint8_t *dst_buf_ptr, size_t dst_buf_len,
                                 const uint8_t *src_ptr, size_t src_len);
  cobs_decode_result cobs_decode(uint8_t *dst_buf_ptr, size_t dst_buf_len,
                                 const uint8_t *src_ptr, size_t src_len);
  
  class CrcNoop {
  public:
    constexpr static size_t size() {
      return 0;
    }

    int value() const {
      return 0;
    }

    void update(const uint8_t *c, size_t length) {
    }
  };

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
      uint8_t *data;
    };

    struct DecodeResult {
      CobsDecodeState status;
      size_t length;
      uint8_t *data;
    };

    uint8_t *readBuffer() {
      return m_readBuffer;
    }

    size_t readBufferSize() {
      return sizeof(m_readBuffer);
    }

    uint8_t *writeBuffer() {
      return m_writePtr;
    }

    size_t writeBufferSize() {
      return sizeof(m_writeBuffer) - overhead(BufferSize);
    }

    Result encodeFrame(const uint8_t *data, size_t length) {
      assert(data);
      assert(length <= BufferSize);

      memcpy(m_writePtr, data, length);
      return encodeFrame(length);
    }
    
    Result encodeFrame(size_t length) {
      assert(length <= BufferSize);

      if(C::size() > 0) {
        m_crc.update(m_writePtr, length);
        auto crc = m_crc.value();
        memcpy(m_writePtr + length, (void *)&crc, sizeof(crc));
      }

      auto result = cobs_encode(m_writeBuffer, sizeof(m_writeBuffer),
                                m_writePtr, length+C::size());
      if(result.status != 0) {
        return { 1, 0, nullptr };
      }

      m_writeBuffer[result.out_len] = 0;

      return { 0, result.out_len + 1, m_writeBuffer };
    }

    DecodeResult readFrameByte(uint8_t byte) {
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
      length--; // Discard null byte

      auto result = cobs_decode((uint8_t *)m_readBuffer, sizeof(m_readBuffer), (const uint8_t *)m_readBuffer, length);
      if(result.status != 0) {
        printf("Status: %i", result.status);
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

    C m_crc;
    uint8_t m_readBuffer[BufferSize + overhead(BufferSize)];
    uint8_t *m_readPos = m_readBuffer;
    uint8_t m_writeBuffer[BufferSize + overhead(BufferSize)];
    uint8_t *m_writePtr = m_writeBuffer + cobsOverhead(BufferSize);
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
  cobs_encode_result cobs_encode(uint8_t *dst_buf_ptr, size_t dst_buf_len,
                                 const uint8_t *src_ptr, size_t src_len)
  {
    cobs_encode_result result = {0, COBS_ENCODE_OK};
    const uint8_t *src_read_ptr = src_ptr;
    const uint8_t *src_end_ptr = src_read_ptr + src_len;
    uint8_t *dst_buf_start_ptr = dst_buf_ptr;
    uint8_t *dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
    uint8_t *dst_code_write_ptr = dst_buf_ptr;
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
  cobs_decode_result cobs_decode(uint8_t *dst_buf_ptr, size_t dst_buf_len,
                                 const uint8_t *src_ptr, size_t src_len)
  {
    cobs_decode_result result = {0, COBS_DECODE_OK};
    const uint8_t *src_read_ptr = src_ptr;
    const uint8_t *src_end_ptr = src_read_ptr + src_len;
    uint8_t *dst_buf_start_ptr = dst_buf_ptr;
    uint8_t *dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
    uint8_t *dst_write_ptr = dst_buf_ptr;
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
