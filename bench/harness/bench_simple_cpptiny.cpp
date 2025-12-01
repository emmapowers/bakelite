/*
 * Benchmark harness for simple.bakelite (cpptiny)
 *
 * Globals declared at file scope to match typical embedded usage.
 * This gives accurate RAM (data+bss) and stack measurements.
 */

#include "simple.h"

/* === Global state (typical embedded pattern) === */

static char g_buffer[64];
static Bakelite::BufferStream g_stream(g_buffer, sizeof(g_buffer));

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

extern "C" void bench_init(void) {
    // BufferStream is initialized at construction
}

/* === Operations under test === */

extern "C" void bench_Simple_pack(void) {
    g_stream.seek(0);
    g_result = g_obj_in.pack(g_stream);
}

extern "C" void bench_Simple_unpack(void) {
    g_stream.seek(0);
    g_result = g_obj_out.unpack(g_stream);
}
