/*
 * Benchmark harness for simple.bakelite (ctiny)
 *
 * Globals declared at file scope to match typical embedded usage.
 * This gives accurate RAM (data+bss) and stack measurements.
 */

#include "simple.h"

/* === Global state (typical embedded pattern) === */

static uint8_t g_buffer[64];
static Bakelite_Buffer g_buf;

/* Test data */
static Simple g_obj_out;
static Simple g_obj_in = {
    .a = 0x12,
    .b = 0x3456,
    .c = 0x789ABCDE,
    .d = -10,
    .e = -1000,
    .f = -100000
};

/* Prevent dead code elimination */
volatile int g_result;

/* === One-time setup (not benchmarked) === */

void bench_init(void) {
    bakelite_buffer_init(&g_buf, g_buffer, sizeof(g_buffer));
}

/* === Operations under test === */

void bench_Simple_pack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Simple_pack(&g_obj_in, &g_buf);
}

void bench_Simple_unpack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Simple_unpack(&g_obj_out, &g_buf);
}
