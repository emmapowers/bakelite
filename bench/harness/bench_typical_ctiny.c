/*
 * Benchmark harness for typical.bakelite (ctiny)
 *
 * Realistic protocol with multiple message types and enum.
 * Globals declared at file scope to match typical embedded usage.
 */

#include "typical.h"

/* === Global state (typical embedded pattern) === */

static uint8_t g_buffer[128];
static Bakelite_Buffer g_buf;

/* Test data - SensorData */
static SensorData g_sensor_out;
static SensorData g_sensor_in = {
    .timestamp = 1234567890,
    .temperature = 2350,   /* 23.5 C */
    .humidity = 6500,      /* 65.0% */
    .pressure = 101325     /* Pa */
};

/* Test data - Command */
static Command g_cmd_out;
static Command g_cmd_in = {
    .opcode = 0x10,
    .param = 0xDEADBEEF
};

/* Test data - Response */
static Response g_resp_out;
static Response g_resp_in = {
    .status = Status_Ok,
    .data = {0xCA, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD, 0xBE, 0xEF,
             0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}
};

/* Prevent dead code elimination */
volatile int g_result;

/* === One-time setup (not benchmarked) === */

void bench_init(void) {
    bakelite_buffer_init(&g_buf, g_buffer, sizeof(g_buffer));
}

/* === Operations under test === */

void bench_SensorData_pack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = SensorData_pack(&g_sensor_in, &g_buf);
}

void bench_SensorData_unpack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = SensorData_unpack(&g_sensor_out, &g_buf);
}

void bench_Command_pack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Command_pack(&g_cmd_in, &g_buf);
}

void bench_Command_unpack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Command_unpack(&g_cmd_out, &g_buf);
}

void bench_Response_pack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Response_pack(&g_resp_in, &g_buf);
}

void bench_Response_unpack(void) {
    bakelite_buffer_reset(&g_buf);
    g_result = Response_unpack(&g_resp_out, &g_buf);
}
