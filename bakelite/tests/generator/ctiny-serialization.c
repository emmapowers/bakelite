#include <stdio.h>
#include <string.h>
#include "struct-ctiny.h"

#define GREATEST_ENABLE_FLOAT
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

TEST simple_struct(void) {
    uint8_t data[256];
    uint8_t heap[256];
    Bakelite_Buffer buf;
    bakelite_buffer_init_with_heap(&buf, data, 256, heap, 256);

    Ack t1 = { .code = 123 };
    ASSERT_EQ(Ack_pack(&t1, &buf), 0);

    ASSERT_EQ(bakelite_buffer_pos(&buf), 1);
    char hex[512];
    hex_string(data, bakelite_buffer_pos(&buf), hex);
    ASSERT_STR_EQ(hex, "7b");

    Ack t2;
    bakelite_buffer_seek(&buf, 0);
    ASSERT_EQ(Ack_unpack(&t2, &buf), 0);

    ASSERT_EQ(t2.code, 123);
    PASS();
}

TEST complex_struct(void) {
    uint8_t data[256];
    uint8_t heap[256];
    Bakelite_Buffer buf;
    bakelite_buffer_init_with_heap(&buf, data, 256, heap, 256);

    TestStruct t1 = {
        .int1 = 5,
        .int2 = -1234,
        .uint1 = 31,
        .uint2 = 1234,
        .float1 = -1.23f,
        .b1 = true,
        .b2 = true,
        .b3 = false,
        .data = {1, 2, 3, 4},
        .str = "hey"
    };
    ASSERT_EQ(TestStruct_pack(&t1, &buf), 0);

    ASSERT_EQ(bakelite_buffer_pos(&buf), 24);
    char hex[512];
    hex_string(data, bakelite_buffer_pos(&buf), hex);
    ASSERT_STR_EQ(hex, "052efbffff1fd204a4709dbf010100010203046865790000");

    TestStruct t2;
    bakelite_buffer_seek(&buf, 0);
    ASSERT_EQ(TestStruct_unpack(&t2, &buf), 0);

    ASSERT_EQ(t2.int1, 5);
    ASSERT_EQ(t2.int2, -1234);
    ASSERT_EQ(t2.uint1, 31);
    ASSERT_EQ(t2.uint2, 1234);
    ASSERT_IN_RANGE(-1.23f, t2.float1, 0.001f);
    ASSERT_EQ(t2.b1, true);
    ASSERT_EQ(t2.b2, true);
    ASSERT_EQ(t2.b3, false);
    ASSERT_STR_EQ(t2.str, "hey");
    PASS();
}

TEST enum_struct(void) {
    uint8_t data[256];
    uint8_t heap[256];
    Bakelite_Buffer buf;
    bakelite_buffer_init_with_heap(&buf, data, 256, heap, 256);

    EnumStruct t1 = {
        .direction = Direction_Left,
        .speed = Speed_Fast
    };
    ASSERT_EQ(EnumStruct_pack(&t1, &buf), 0);

    ASSERT_EQ(bakelite_buffer_pos(&buf), 2);
    char hex[512];
    hex_string(data, bakelite_buffer_pos(&buf), hex);
    ASSERT_STR_EQ(hex, "02ff");

    EnumStruct t2;
    bakelite_buffer_seek(&buf, 0);
    ASSERT_EQ(EnumStruct_unpack(&t2, &buf), 0);

    ASSERT_EQ(t2.direction, Direction_Left);
    ASSERT_EQ(t2.speed, Speed_Fast);
    PASS();
}

TEST nested_struct(void) {
    uint8_t data[256];
    uint8_t heap[256];
    Bakelite_Buffer buf;
    bakelite_buffer_init_with_heap(&buf, data, 256, heap, 256);

    NestedStruct t1 = {
        .a = { .b1 = true, .b2 = false },
        .b = { .num = 127 },
        .num = -4
    };
    ASSERT_EQ(NestedStruct_pack(&t1, &buf), 0);

    ASSERT_EQ(bakelite_buffer_pos(&buf), 4);
    char hex[512];
    hex_string(data, bakelite_buffer_pos(&buf), hex);
    ASSERT_STR_EQ(hex, "01007ffc");

    NestedStruct t2;
    bakelite_buffer_seek(&buf, 0);
    ASSERT_EQ(NestedStruct_unpack(&t2, &buf), 0);

    ASSERT_EQ(t2.a.b1, true);
    ASSERT_EQ(t2.a.b2, false);
    ASSERT_EQ(t2.b.num, 127);
    ASSERT_EQ(t2.num, -4);
    PASS();
}

TEST deeply_nested_struct(void) {
    uint8_t data[256];
    uint8_t heap[256];
    Bakelite_Buffer buf;
    bakelite_buffer_init_with_heap(&buf, data, 256, heap, 256);

    DeeplyNestedStruct t1 = {
        .c = { .a = { .b1 = false, .b2 = true } }
    };
    ASSERT_EQ(DeeplyNestedStruct_pack(&t1, &buf), 0);

    ASSERT_EQ(bakelite_buffer_pos(&buf), 2);
    char hex[512];
    hex_string(data, bakelite_buffer_pos(&buf), hex);
    ASSERT_STR_EQ(hex, "0001");

    DeeplyNestedStruct t2;
    bakelite_buffer_seek(&buf, 0);
    ASSERT_EQ(DeeplyNestedStruct_unpack(&t2, &buf), 0);

    ASSERT_EQ(t2.c.a.b1, false);
    ASSERT_EQ(t2.c.a.b2, true);
    PASS();
}

TEST array_struct(void) {
    uint8_t data[256];
    uint8_t heap[256];
    Bakelite_Buffer buf;
    bakelite_buffer_init_with_heap(&buf, data, 256, heap, 256);

    ArrayStruct t1 = {
        .a = { Direction_Left, Direction_Right, Direction_Down },
        .b = { { .code = 127 }, { .code = 64 } },
        .c = { "abc", "def", "ghi" }
    };
    ASSERT_EQ(ArrayStruct_pack(&t1, &buf), 0);

    ASSERT_EQ(bakelite_buffer_pos(&buf), 17);
    char hex[512];
    hex_string(data, bakelite_buffer_pos(&buf), hex);
    ASSERT_STR_EQ(hex, "0203017f40616263006465660067686900");

    ArrayStruct t2;
    bakelite_buffer_seek(&buf, 0);
    ASSERT_EQ(ArrayStruct_unpack(&t2, &buf), 0);

    ASSERT_EQ(t2.a[0], Direction_Left);
    ASSERT_EQ(t2.a[1], Direction_Right);
    ASSERT_EQ(t2.a[2], Direction_Down);
    ASSERT_EQ(t2.b[0].code, 127);
    ASSERT_EQ(t2.b[1].code, 64);
    ASSERT_STR_EQ(t2.c[0], "abc");
    ASSERT_STR_EQ(t2.c[1], "def");
    ASSERT_STR_EQ(t2.c[2], "ghi");
    PASS();
}

TEST variable_length_struct(void) {
    uint8_t data[256];
    uint8_t heap[256];
    Bakelite_Buffer buf;
    bakelite_buffer_init_with_heap(&buf, data, 256, heap, 256);

    uint8_t byteData[11] = { 0x68, 0x65, 0x6C, 0x6C, 0x6F,
        0,
        0x57, 0x6F, 0x72, 0x6C, 0x64 };
    uint8_t numbers[4] = { 1, 2, 3, 4 };

    VariableLength t1;
    t1.a.data = byteData;
    t1.a.size = 11;
    t1.b = "This is a test string!";
    t1.c.data = numbers;
    t1.c.size = 4;

    ASSERT_EQ(VariableLength_pack(&t1, &buf), 0);

    /* Verify data was packed correctly */
    ASSERT(bakelite_buffer_pos(&buf) > 0);

    VariableLength t2;
    bakelite_buffer_seek(&buf, 0);
    ASSERT_EQ(VariableLength_unpack(&t2, &buf), 0);

    ASSERT_EQ(t2.a.size, 11);
    ASSERT_MEM_EQ(t2.a.data, byteData, 11);
    ASSERT_STR_EQ(t2.b, "This is a test string!");
    ASSERT_EQ(t2.c.size, 4);
    ASSERT_MEM_EQ(t2.c.data, numbers, 4);
    PASS();
}

SUITE(serialization) {
    RUN_TEST(simple_struct);
    RUN_TEST(complex_struct);
    RUN_TEST(enum_struct);
    RUN_TEST(nested_struct);
    RUN_TEST(deeply_nested_struct);
    RUN_TEST(array_struct);
    RUN_TEST(variable_length_struct);
}
