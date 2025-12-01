/*
 * Benchmark harness for framing.bakelite (cpptiny)
 *
 * Tests struct serialization for protocol with framing.
 * Globals declared at file scope to match typical embedded usage.
 */

#include "framing.h"

/* === Global state (typical embedded pattern) === */

static char g_buffer[128];
static Bakelite::BufferStream g_stream(g_buffer, sizeof(g_buffer));

/* Test data - Payload */
static Payload g_payload_out;
static Payload g_payload_in = {
    .id = 0x42,
    .data = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
             0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
             0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
             0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F}
};

/* Prevent dead code elimination */
volatile int g_result;

/* === One-time setup (not benchmarked) === */

extern "C" void bench_init(void) {
    // BufferStream is initialized at construction
}

/* === Operations under test === */

extern "C" void bench_Payload_pack(void) {
    g_stream.seek(0);
    g_result = g_payload_in.pack(g_stream);
}

extern "C" void bench_Payload_unpack(void) {
    g_stream.seek(0);
    g_result = g_payload_out.unpack(g_stream);
}
