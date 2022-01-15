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