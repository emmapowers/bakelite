#include <stdio.h>
#include <string.h>
#include "bakelite-ctiny.h"
#include "greatest.h"

/* Helper to convert buffer to hex string */
static void hex_string(const uint8_t *data, size_t length, char *out) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < length; i++) {
        out[i * 2] = hex[(data[i] >> 4) & 0x0f];
        out[i * 2 + 1] = hex[data[i] & 0x0f];
    }
    out[length * 2] = '\0';
}

/* Helper to write a complete frame byte-by-byte */
static Bakelite_DecodeResult write_frame(Bakelite_CobsFramer *framer, const uint8_t *data,
                                          size_t length, Bakelite_DecodeState expected_state) {
    Bakelite_DecodeResult result;
    for (size_t i = 0; i < length - 1; i++) {
        result = bakelite_framer_read_byte(framer, data[i]);
        if (result.status != BAKELITE_DECODE_NOT_READY) {
            return result;
        }
    }
    result = bakelite_framer_read_byte(framer, data[length - 1]);
    return result;
}

/* COBS encode tests */
TEST encode_frame(void) {
    const size_t buffSize = 259;
    uint8_t buffer[buffSize + BAKELITE_COBS_ENCODE_SRC_OFFSET(buffSize)];
    memset(buffer, 0xFF, sizeof(buffer));
    uint8_t *srcPtr = buffer + BAKELITE_COBS_ENCODE_SRC_OFFSET(buffSize);

    /* 256 bytes of 0xEE with 0x00 at start and end, plus 0xAA 0xBB */
    srcPtr[0] = 0x00;
    for (int i = 1; i <= 254; i++) {
        srcPtr[i] = 0xEE;
    }
    srcPtr[255] = 0x00;
    srcPtr[256] = 0xAA;
    srcPtr[257] = 0xBB;

    Bakelite_CobsEncodeResult result = bakelite_cobs_encode(buffer, sizeof(buffer), srcPtr, 258);
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.out_len, 260);

    memset(buffer + result.out_len, 0xFF, sizeof(buffer) - result.out_len);
    char hex[600];
    hex_string(buffer, sizeof(buffer), hex);
    ASSERT_STR_EQ(hex, "01ffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee0103aabbff");
    PASS();
}

TEST encode_frame_one_byte(void) {
    const size_t buffSize = 259;
    uint8_t buffer[buffSize + BAKELITE_COBS_ENCODE_SRC_OFFSET(buffSize)];
    memset(buffer, 0xFF, sizeof(buffer));
    uint8_t *srcPtr = buffer + BAKELITE_COBS_ENCODE_SRC_OFFSET(buffSize);
    srcPtr[0] = 0x22;

    Bakelite_CobsEncodeResult result = bakelite_cobs_encode(buffer, sizeof(buffer), srcPtr, 1);
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.out_len, 2);

    char hex[16];
    hex_string(buffer, result.out_len, hex);
    ASSERT_STR_EQ(hex, "0222");
    PASS();
}

TEST decode_frame(void) {
    uint8_t buffer[260];
    memset(buffer, 0xFF, sizeof(buffer));

    buffer[0] = 0x01;
    buffer[1] = 0xFF;
    for (int i = 0; i < 254; i++) {
        buffer[i + 2] = 0xEE;
    }
    buffer[256] = 0x01;
    buffer[257] = 0x03;
    buffer[258] = 0xAA;
    buffer[259] = 0xBB;

    Bakelite_CobsDecodeResult result = bakelite_cobs_decode(buffer, sizeof(buffer), buffer, sizeof(buffer));
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.out_len, 258);

    memset(buffer + result.out_len, 0xFF, sizeof(buffer) - result.out_len);
    char hex[600];
    hex_string(buffer, sizeof(buffer), hex);
    ASSERT_STR_EQ(hex, "00eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee00aabbffff");
    PASS();
}

/* Framer encode tests */
TEST framer_encode(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_FramerResult result = bakelite_framer_encode_copy(&framer, (const uint8_t *)"\x11\x22\x33\x44", 4);
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.length, 6);

    char hex[64];
    hex_string(result.data, result.length, hex);
    ASSERT_STR_EQ(hex, "051122334400");
    PASS();
}

TEST framer_encode_zero_length(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_FramerResult result = bakelite_framer_encode_copy(&framer, (const uint8_t *)"", 0);
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.length, 2);

    char hex[16];
    hex_string(result.data, result.length, hex);
    ASSERT_STR_EQ(hex, "0100");
    PASS();
}

TEST framer_decode(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x05\x11\x22\x33\x44\x00", 6, BAKELITE_DECODE_OK);
    ASSERT_EQ(result.status, BAKELITE_DECODE_OK);
    ASSERT_EQ(result.length, 4);

    char hex[64];
    hex_string(result.data, result.length, hex);
    ASSERT_STR_EQ(hex, "11223344");
    PASS();
}

TEST framer_encode_one_byte(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_FramerResult result = bakelite_framer_encode_copy(&framer, (const uint8_t *)"\x22", 1);
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.length, 3);

    char hex[16];
    hex_string(result.data, result.length, hex);
    ASSERT_STR_EQ(hex, "022200");
    PASS();
}

TEST framer_decode_zero_length(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_DecodeResult result = bakelite_framer_read_byte(&framer, 0x01);
    ASSERT_EQ(result.status, BAKELITE_DECODE_NOT_READY);
    result = bakelite_framer_read_byte(&framer, 0x00);
    ASSERT_EQ(result.status, BAKELITE_DECODE_OK);
    ASSERT_EQ(result.length, 0);
    PASS();
}

TEST framer_decode_more_bytes(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x05\x11\x22\x33\x44\x00", 6, BAKELITE_DECODE_OK);
    ASSERT_EQ(result.length, 4);

    result = write_frame(&framer, (const uint8_t *)"\x05\x11\x22\x33\x44\x00", 6, BAKELITE_DECODE_OK);
    ASSERT_EQ(result.length, 4);

    char hex[64];
    hex_string(result.data, result.length, hex);
    ASSERT_STR_EQ(hex, "11223344");

    result = bakelite_framer_read_byte(&framer, 0x05);
    ASSERT_EQ(result.status, BAKELITE_DECODE_NOT_READY);
    PASS();
}

TEST framer_decode_two_null_bytes(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x05\x11\x22\x33\x44\x00", 6, BAKELITE_DECODE_OK);
    ASSERT_EQ(result.length, 4);

    char hex[64];
    hex_string(result.data, result.length, hex);
    ASSERT_STR_EQ(hex, "11223344");

    result = bakelite_framer_read_byte(&framer, 0x00);
    ASSERT_EQ(result.status, BAKELITE_DECODE_FAILURE);
    PASS();
}

TEST framer_buffer_overrun(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(2, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 2, BAKELITE_CRC_NONE);

    Bakelite_DecodeResult result = bakelite_framer_read_byte(&framer, 0x05);
    ASSERT_EQ(result.status, BAKELITE_DECODE_NOT_READY);
    result = bakelite_framer_read_byte(&framer, 0x11);
    ASSERT_EQ(result.status, BAKELITE_DECODE_NOT_READY);
    result = bakelite_framer_read_byte(&framer, 0x22);
    ASSERT_EQ(result.status, BAKELITE_DECODE_NOT_READY);
    result = bakelite_framer_read_byte(&framer, 0x33);
    ASSERT_EQ(result.status, BAKELITE_DECODE_BUFFER_OVERRUN);
    PASS();
}

TEST framer_decode_failure(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x01\x11\x22\x33\x44\x00", 6, BAKELITE_DECODE_FAILURE);
    ASSERT_EQ(result.status, BAKELITE_DECODE_FAILURE);
    PASS();
}

TEST framer_decode_failure_2(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x10\x11\x22\x33\x44\x00", 6, BAKELITE_DECODE_FAILURE);
    ASSERT_EQ(result.status, BAKELITE_DECODE_FAILURE);
    PASS();
}

TEST framer_roundtrip(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 0)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_NONE);

    Bakelite_FramerResult encodeResult = bakelite_framer_encode_copy(&framer, (const uint8_t *)"\x11\x22\x33\x44", 4);
    ASSERT_EQ(encodeResult.status, 0);

    for (size_t i = 0; i < encodeResult.length - 1; i++) {
        Bakelite_DecodeResult decodeResult = bakelite_framer_read_byte(&framer, encodeResult.data[i]);
        ASSERT_EQ(decodeResult.status, BAKELITE_DECODE_NOT_READY);
    }

    Bakelite_DecodeResult decodeResult = bakelite_framer_read_byte(&framer, encodeResult.data[encodeResult.length - 1]);
    ASSERT_EQ(decodeResult.status, BAKELITE_DECODE_OK);
    ASSERT_EQ(decodeResult.length, 4);

    char hex[64];
    hex_string(decodeResult.data, decodeResult.length, hex);
    ASSERT_STR_EQ(hex, "11223344");
    PASS();
}

/* CRC encode tests */
TEST framer_encode_crc8(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 1)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_8);

    Bakelite_FramerResult result = bakelite_framer_encode_copy(&framer, (const uint8_t *)"\x11\x22\x33\x44", 4);
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.length, 7);

    char hex[64];
    hex_string(result.data, result.length, hex);
    ASSERT_STR_EQ(hex, "0611223344f900");
    PASS();
}

TEST framer_encode_crc16(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 2)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_16);

    Bakelite_FramerResult result = bakelite_framer_encode_copy(&framer, (const uint8_t *)"\x11\x22\x33\x44", 4);
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.length, 8);

    char hex[64];
    hex_string(result.data, result.length, hex);
    ASSERT_STR_EQ(hex, "0711223344b1f500");
    PASS();
}

TEST framer_encode_crc32(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 4)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_32);

    Bakelite_FramerResult result = bakelite_framer_encode_copy(&framer, (const uint8_t *)"\x11\x22\x33\x44", 4);
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.length, 10);

    char hex[64];
    hex_string(result.data, result.length, hex);
    ASSERT_STR_EQ(hex, "0911223344d19df27700");
    PASS();
}

/* CRC decode tests */
TEST framer_decode_crc8(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 1)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_8);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x06\x11\x22\x33\x44\xf9\x00", 7, BAKELITE_DECODE_OK);
    ASSERT_EQ(result.status, BAKELITE_DECODE_OK);
    PASS();
}

TEST framer_decode_crc8_failure(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 1)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_8);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x06\xFF\x22\x33\x44\xf9\x00", 7, BAKELITE_DECODE_CRC_FAILURE);
    ASSERT_EQ(result.status, BAKELITE_DECODE_CRC_FAILURE);
    PASS();
}

TEST framer_decode_crc16(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 2)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_16);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x07\x11\x22\x33\x44\xb1\xf5\x00", 8, BAKELITE_DECODE_OK);
    ASSERT_EQ(result.status, BAKELITE_DECODE_OK);
    PASS();
}

TEST framer_decode_crc16_failure(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 2)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_16);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x07\xFF\x22\x33\x44\xb1\xf5\x00", 8, BAKELITE_DECODE_CRC_FAILURE);
    ASSERT_EQ(result.status, BAKELITE_DECODE_CRC_FAILURE);
    PASS();
}

TEST framer_decode_crc32(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 4)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_32);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x09\x11\x22\x33\x44\xd1\x9d\xf2\x77\x00", 10, BAKELITE_DECODE_OK);
    ASSERT_EQ(result.status, BAKELITE_DECODE_OK);
    PASS();
}

TEST framer_decode_crc32_failure(void) {
    uint8_t buffer[BAKELITE_FRAMER_BUFFER_SIZE(256, 4)];
    Bakelite_CobsFramer framer;
    bakelite_framer_init(&framer, buffer, sizeof(buffer), 256, BAKELITE_CRC_32);

    Bakelite_DecodeResult result = write_frame(&framer, (const uint8_t *)"\x09\xFF\x22\x33\x44\xd1\x9d\xf2\x77\x00", 10, BAKELITE_DECODE_CRC_FAILURE);
    ASSERT_EQ(result.status, BAKELITE_DECODE_CRC_FAILURE);
    PASS();
}

SUITE(framing) {
    RUN_TEST(encode_frame);
    RUN_TEST(encode_frame_one_byte);
    RUN_TEST(decode_frame);
    RUN_TEST(framer_encode);
    RUN_TEST(framer_encode_zero_length);
    RUN_TEST(framer_decode);
    RUN_TEST(framer_encode_one_byte);
    RUN_TEST(framer_decode_zero_length);
    RUN_TEST(framer_decode_more_bytes);
    RUN_TEST(framer_decode_two_null_bytes);
    RUN_TEST(framer_buffer_overrun);
    RUN_TEST(framer_decode_failure);
    RUN_TEST(framer_decode_failure_2);
    RUN_TEST(framer_roundtrip);
    RUN_TEST(framer_encode_crc8);
    RUN_TEST(framer_encode_crc16);
    RUN_TEST(framer_encode_crc32);
    RUN_TEST(framer_decode_crc8);
    RUN_TEST(framer_decode_crc8_failure);
    RUN_TEST(framer_decode_crc16);
    RUN_TEST(framer_decode_crc16_failure);
    RUN_TEST(framer_decode_crc32);
    RUN_TEST(framer_decode_crc32_failure);
}
