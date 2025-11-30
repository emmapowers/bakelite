#include <stdio.h>
#include <string.h>
#include "proto-ctiny.h"
#include "greatest.h"

/* Declare suites from other files */
SUITE_EXTERN(serialization);
SUITE_EXTERN(framing);

/* Helper to convert buffer to hex string */
static void hex_string(const uint8_t *data, size_t length, char *out) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < length; i++) {
        out[i * 2] = hex[(data[i] >> 4) & 0x0f];
        out[i * 2 + 1] = hex[data[i] & 0x0f];
    }
    out[length * 2] = '\0';
}

/* Test stream for simulating I/O */
typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t pos;
    bool blocking;
} TestStream;

static TestStream test_stream;

static int stream_read(void) {
    if (test_stream.blocking) {
        return -1;
    }
    if (test_stream.pos >= test_stream.size) {
        return -2;
    }
    return test_stream.buffer[test_stream.pos++];
}

static size_t stream_write(const uint8_t *data, size_t length) {
    size_t end_pos = test_stream.pos + length;
    if (end_pos > test_stream.size) {
        return 0;
    }
    memcpy(test_stream.buffer + test_stream.pos, data, length);
    test_stream.pos += length;
    return length;
}

static void stream_init(uint8_t *buffer, size_t size) {
    test_stream.buffer = buffer;
    test_stream.size = size;
    test_stream.pos = 0;
    test_stream.blocking = false;
}

static void stream_reset(void) {
    memset(test_stream.buffer, 0, test_stream.size);
    test_stream.pos = 0;
    test_stream.blocking = false;
}

TEST proto_send_message(void) {
    uint8_t data[256];
    stream_init(data, 256);

    Protocol protocol;
    Protocol_init(&protocol, stream_read, stream_write);

    Ack ack = { .code = 0x22 };
    Protocol_send_Ack(&protocol, &ack);

    ASSERT_EQ(test_stream.pos, 5);
    char hex[64];
    hex_string(data, test_stream.pos, hex);
    ASSERT_STR_EQ(hex, "040222c400");

    size_t length = test_stream.pos;
    test_stream.pos = 0;

    for (; test_stream.pos < length - 1;) {
        ASSERT_EQ(Protocol_poll(&protocol), Protocol_NoMessage);
    }
    Protocol_Message msg = Protocol_poll(&protocol);
    ASSERT_EQ(msg, Protocol_Ack);

    Ack result;
    ASSERT_EQ(Protocol_decode_Ack(&protocol, &result, NULL, 0), 0);
    ASSERT_EQ(result.code, 0x22);
    PASS();
}

TEST proto_send_larger_message(void) {
    uint8_t data[256];
    stream_init(data, 256);

    Protocol protocol;
    Protocol_init(&protocol, stream_read, stream_write);

    TestMessage input = {
        .a = 0x22,
        .b = -1234,
        .status = false,
        .message = "Hello World!"
    };
    Protocol_send_TestMessage(&protocol, &input);

    ASSERT_EQ(test_stream.pos, 26);
    char hex[64];
    hex_string(data, test_stream.pos, hex);
    ASSERT_STR_EQ(hex, "0701222efbffff0d48656c6c6f20576f726c6421010101026200");

    size_t length = test_stream.pos;
    test_stream.pos = 0;

    for (; test_stream.pos < length - 1;) {
        ASSERT_EQ(Protocol_poll(&protocol), Protocol_NoMessage);
    }
    Protocol_Message msg = Protocol_poll(&protocol);
    ASSERT_EQ(msg, Protocol_TestMessage);

    TestMessage result;
    ASSERT_EQ(Protocol_decode_TestMessage(&protocol, &result, NULL, 0), 0);
    ASSERT_EQ(result.a, 0x22);
    ASSERT_EQ(result.b, -1234);
    ASSERT_EQ(result.status, false);
    ASSERT_STR_EQ(result.message, "Hello World!");
    PASS();
}

TEST proto_decode_wrong_message(void) {
    uint8_t data[256];
    stream_init(data, 256);

    Protocol protocol;
    Protocol_init(&protocol, stream_read, stream_write);

    Ack ack = { .code = 0x22 };
    Protocol_send_Ack(&protocol, &ack);

    ASSERT_EQ(test_stream.pos, 5);
    char hex[64];
    hex_string(data, test_stream.pos, hex);
    ASSERT_STR_EQ(hex, "040222c400");

    size_t length = test_stream.pos;
    test_stream.pos = 0;

    for (; test_stream.pos < length - 1;) {
        ASSERT_EQ(Protocol_poll(&protocol), Protocol_NoMessage);
    }
    Protocol_Message msg = Protocol_poll(&protocol);
    ASSERT_EQ(msg, Protocol_Ack);

    /* Try to decode as wrong message type */
    TestMessage result;
    ASSERT_EQ(Protocol_decode_TestMessage(&protocol, &result, NULL, 0), -1);
    PASS();
}

TEST proto_receive_no_dynamic_memory(void) {
    uint8_t data[256];
    stream_init(data, 256);

    Protocol protocol;
    Protocol_init(&protocol, stream_read, stream_write);

    ArrayMessage msg;
    int32_t numbers[3] = {1234, -1234, 456};
    msg.numbers.data = numbers;
    msg.numbers.size = 3;
    Protocol_send_ArrayMessage(&protocol, &msg);

    ASSERT_EQ(test_stream.pos, 17);
    char hex[64];
    hex_string(data, test_stream.pos, hex);
    ASSERT_STR_EQ(hex, "050303d20401072efbffffc8010102bb00");

    size_t length = test_stream.pos;
    test_stream.pos = 0;

    for (; test_stream.pos < length - 1;) {
        ASSERT_EQ(Protocol_poll(&protocol), Protocol_NoMessage);
    }
    Protocol_Message msgId = Protocol_poll(&protocol);
    ASSERT_EQ(msgId, Protocol_ArrayMessage);

    /* Decode without providing heap - should fail */
    ArrayMessage result;
    ASSERT_EQ(Protocol_decode_ArrayMessage(&protocol, &result, NULL, 0), -4);
    PASS();
}

TEST proto_receive_dynamic(void) {
    uint8_t data[256];
    stream_init(data, 256);

    Protocol protocol;
    Protocol_init(&protocol, stream_read, stream_write);

    ArrayMessage msg;
    int32_t numbers[3] = {1234, -1234, 456};
    msg.numbers.data = numbers;
    msg.numbers.size = 3;
    Protocol_send_ArrayMessage(&protocol, &msg);

    ASSERT_EQ(test_stream.pos, 17);
    char hex[64];
    hex_string(data, test_stream.pos, hex);
    ASSERT_STR_EQ(hex, "050303d20401072efbffffc8010102bb00");

    size_t length = test_stream.pos;
    test_stream.pos = 0;

    for (; test_stream.pos < length - 1;) {
        ASSERT_EQ(Protocol_poll(&protocol), Protocol_NoMessage);
    }
    Protocol_Message msgId = Protocol_poll(&protocol);
    ASSERT_EQ(msgId, Protocol_ArrayMessage);

    ArrayMessage result;
    uint8_t heap[64];
    ASSERT_EQ(Protocol_decode_ArrayMessage(&protocol, &result, heap, sizeof(heap)), 0);
    ASSERT_EQ(result.numbers.size, 3);
    ASSERT_EQ(((int32_t *)result.numbers.data)[0], 1234);
    ASSERT_EQ(((int32_t *)result.numbers.data)[1], -1234);
    ASSERT_EQ(((int32_t *)result.numbers.data)[2], 456);
    PASS();
}

SUITE(protocol) {
    RUN_TEST(proto_send_message);
    RUN_TEST(proto_send_larger_message);
    RUN_TEST(proto_decode_wrong_message);
    RUN_TEST(proto_receive_no_dynamic_memory);
    RUN_TEST(proto_receive_dynamic);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(serialization);
    RUN_SUITE(framing);
    RUN_SUITE(protocol);
    GREATEST_MAIN_END();
}
