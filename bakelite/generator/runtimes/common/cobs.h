/*
 * Bakelite COBS Functions (C99)
 *
 * COBS encode/decode functions are Copyright (c) 2010 Craig McQueen
 * Licensed under the MIT license (see end of file).
 * Source: https://github.com/cmcqueen/cobs-c
 */

#ifndef BAKELITE_COMMON_COBS_H
#define BAKELITE_COMMON_COBS_H

#include <stdint.h>
#include <stddef.h>

/* COBS buffer size calculations */
#define BAKELITE_COBS_ENCODE_DST_BUF_LEN_MAX(SRC_LEN) ((SRC_LEN) + (((SRC_LEN) + 253u) / 254u))
#define BAKELITE_COBS_DECODE_DST_BUF_LEN_MAX(SRC_LEN) (((SRC_LEN) == 0) ? 0u : ((SRC_LEN) - 1u))
#define BAKELITE_COBS_OVERHEAD(BUF_SIZE)              (((BUF_SIZE) + 253u) / 254u)
#define BAKELITE_COBS_ENCODE_SRC_OFFSET(SRC_LEN)      (((SRC_LEN) + 253u) / 254u)

/* Legacy macro names for backward compatibility */
#define COBS_ENCODE_DST_BUF_LEN_MAX(SRC_LEN) BAKELITE_COBS_ENCODE_DST_BUF_LEN_MAX(SRC_LEN)
#define COBS_DECODE_DST_BUF_LEN_MAX(SRC_LEN) BAKELITE_COBS_DECODE_DST_BUF_LEN_MAX(SRC_LEN)
#define COBS_ENCODE_SRC_OFFSET(SRC_LEN)      BAKELITE_COBS_ENCODE_SRC_OFFSET(SRC_LEN)

/* COBS encode/decode status */
typedef enum {
    BAKELITE_COBS_ENCODE_OK = 0x00,
    BAKELITE_COBS_ENCODE_NULL_POINTER = 0x01,
    BAKELITE_COBS_ENCODE_OUT_BUFFER_OVERFLOW = 0x02
} Bakelite_CobsEncodeStatus;

typedef enum {
    BAKELITE_COBS_DECODE_OK = 0x00,
    BAKELITE_COBS_DECODE_NULL_POINTER = 0x01,
    BAKELITE_COBS_DECODE_OUT_BUFFER_OVERFLOW = 0x02,
    BAKELITE_COBS_DECODE_ZERO_BYTE_IN_INPUT = 0x04,
    BAKELITE_COBS_DECODE_INPUT_TOO_SHORT = 0x08
} Bakelite_CobsDecodeStatus;

/* COBS encode/decode results */
typedef struct {
    size_t out_len;
    int status;
} Bakelite_CobsEncodeResult;

typedef struct {
    size_t out_len;
    int status;
} Bakelite_CobsDecodeResult;

/*
 * COBS-encode a string of input bytes.
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
static inline Bakelite_CobsEncodeResult bakelite_cobs_encode(void *dst_buf_ptr, size_t dst_buf_len,
                                                             const void *src_ptr, size_t src_len) {
    Bakelite_CobsEncodeResult result = {0, BAKELITE_COBS_ENCODE_OK};
    const uint8_t *src_read_ptr = (const uint8_t *)src_ptr;
    const uint8_t *src_end_ptr = src_read_ptr + src_len;
    uint8_t *dst_buf_start_ptr = (uint8_t *)dst_buf_ptr;
    uint8_t *dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
    uint8_t *dst_code_write_ptr = (uint8_t *)dst_buf_ptr;
    uint8_t *dst_write_ptr = dst_code_write_ptr + 1;
    uint8_t src_byte = 0;
    uint8_t search_len = 1;

    if ((dst_buf_ptr == NULL) || (src_ptr == NULL)) {
        result.status = BAKELITE_COBS_ENCODE_NULL_POINTER;
        return result;
    }

    if (src_len != 0) {
        for (;;) {
            if (dst_write_ptr >= dst_buf_end_ptr) {
                result.status |= BAKELITE_COBS_ENCODE_OUT_BUFFER_OVERFLOW;
                break;
            }

            src_byte = *src_read_ptr++;
            if (src_byte == 0) {
                *dst_code_write_ptr = search_len;
                dst_code_write_ptr = dst_write_ptr++;
                search_len = 1;
                if (src_read_ptr >= src_end_ptr) {
                    break;
                }
            } else {
                *dst_write_ptr++ = src_byte;
                search_len++;
                if (src_read_ptr >= src_end_ptr) {
                    break;
                }
                if (search_len == 0xFF) {
                    *dst_code_write_ptr = search_len;
                    dst_code_write_ptr = dst_write_ptr++;
                    search_len = 1;
                }
            }
        }
    }

    if (dst_code_write_ptr >= dst_buf_end_ptr) {
        result.status |= BAKELITE_COBS_ENCODE_OUT_BUFFER_OVERFLOW;
        dst_write_ptr = dst_buf_end_ptr;
    } else {
        *dst_code_write_ptr = search_len;
    }

    result.out_len = (size_t)(dst_write_ptr - dst_buf_start_ptr);
    return result;
}

/*
 * Decode a COBS byte string.
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
static inline Bakelite_CobsDecodeResult bakelite_cobs_decode(void *dst_buf_ptr, size_t dst_buf_len,
                                                             const void *src_ptr, size_t src_len) {
    Bakelite_CobsDecodeResult result = {0, BAKELITE_COBS_DECODE_OK};
    const uint8_t *src_read_ptr = (const uint8_t *)src_ptr;
    const uint8_t *src_end_ptr = src_read_ptr + src_len;
    uint8_t *dst_buf_start_ptr = (uint8_t *)dst_buf_ptr;
    uint8_t *dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
    uint8_t *dst_write_ptr = (uint8_t *)dst_buf_ptr;
    size_t remaining_bytes;
    uint8_t src_byte;
    uint8_t i;
    uint8_t len_code;

    if ((dst_buf_ptr == NULL) || (src_ptr == NULL)) {
        result.status = BAKELITE_COBS_DECODE_NULL_POINTER;
        return result;
    }

    if (src_len != 0) {
        for (;;) {
            len_code = *src_read_ptr++;
            if (len_code == 0) {
                result.status |= BAKELITE_COBS_DECODE_ZERO_BYTE_IN_INPUT;
                break;
            }
            len_code--;

            remaining_bytes = (size_t)(src_end_ptr - src_read_ptr);
            if (len_code > remaining_bytes) {
                result.status |= BAKELITE_COBS_DECODE_INPUT_TOO_SHORT;
                len_code = (uint8_t)remaining_bytes;
            }

            remaining_bytes = (size_t)(dst_buf_end_ptr - dst_write_ptr);
            if (len_code > remaining_bytes) {
                result.status |= BAKELITE_COBS_DECODE_OUT_BUFFER_OVERFLOW;
                len_code = (uint8_t)remaining_bytes;
            }

            for (i = len_code; i != 0; i--) {
                src_byte = *src_read_ptr++;
                if (src_byte == 0) {
                    result.status |= BAKELITE_COBS_DECODE_ZERO_BYTE_IN_INPUT;
                }
                *dst_write_ptr++ = src_byte;
            }

            if (src_read_ptr >= src_end_ptr) {
                break;
            }

            if (len_code != 0xFE) {
                if (dst_write_ptr >= dst_buf_end_ptr) {
                    result.status |= BAKELITE_COBS_DECODE_OUT_BUFFER_OVERFLOW;
                    break;
                }
                *dst_write_ptr++ = 0;
            }
        }
    }

    result.out_len = (size_t)(dst_write_ptr - dst_buf_start_ptr);
    return result;
}

/*
 * MIT License for COBS encode/decode functions:
 *
 * Copyright (c) 2010 Craig McQueen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#endif /* BAKELITE_COMMON_COBS_H */
