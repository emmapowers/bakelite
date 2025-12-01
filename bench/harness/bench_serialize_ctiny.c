/*
 * Benchmark harness for serialize.bakelite (ctiny)
 *
 * Globals declared at file scope to match typical embedded usage.
 * This gives accurate RAM (data+bss) and stack measurements.
 */

#include "serialize.h"

/* === Global state (typical embedded pattern) === */

static uint8_t g_buffer[256];
static Bakelite_Buffer g_buf;

/* Test data - Primitives */
static Primitives g_prim_out;
static Primitives g_prim_in = {
    .u8 = 0x12,
    .u16 = 0x3456,
    .u32 = 0x789ABCDE,
    .i8 = -10,
    .i16 = -1000,
    .i32 = -100000,
    .f32 = 3.14159f,
    .flag = 1
};

/* Test data - Arrays */
static Arrays g_arr_out;
static Arrays g_arr_in = {
    .bytes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10},
    .nums = {100, 200, 300, 400, 500, 600, 700, 800},
    .str = "Hello, Bakelite!"
};

/* Test data - Nested */
static Nested g_nest_out;
static Nested g_nest_in = {
    .header = {
        .u8 = 0xAB,
        .u16 = 0xCDEF,
        .u32 = 0x12345678,
        .i8 = -5,
        .i16 = -500,
        .i32 = -50000,
        .f32 = 2.71828f,
        .flag = 0
    },
    .data = {
        .bytes = {0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87,
                  0x78, 0x69, 0x5A, 0x4B, 0x3C, 0x2D, 0x1E, 0x0F},
        .nums = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000},
        .str = "Nested struct test"
    }
};

/* Prevent dead code elimination */
volatile int g_result;

/* === One-time setup (not benchmarked) === */

void bench_init(void) {
    bakelite_buffer_init(&g_buf, g_buffer, sizeof(g_buffer));
}

/* === Operations under test === */

void bench_Primitives_pack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Primitives_pack(&g_prim_in, &g_buf);
}

void bench_Primitives_unpack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Primitives_unpack(&g_prim_out, &g_buf);
}

void bench_Arrays_pack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Arrays_pack(&g_arr_in, &g_buf);
}

void bench_Arrays_unpack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Arrays_unpack(&g_arr_out, &g_buf);
}

void bench_Nested_pack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Nested_pack(&g_nest_in, &g_buf);
}

void bench_Nested_unpack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Nested_unpack(&g_nest_out, &g_buf);
}
