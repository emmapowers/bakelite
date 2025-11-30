/* Write primitives */
static inline int bakelite_write_bool(Bakelite_Buffer *buf, bool val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_int8(Bakelite_Buffer *buf, int8_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_uint8(Bakelite_Buffer *buf, uint8_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_int16(Bakelite_Buffer *buf, int16_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_uint16(Bakelite_Buffer *buf, uint16_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_int32(Bakelite_Buffer *buf, int32_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_uint32(Bakelite_Buffer *buf, uint32_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_int64(Bakelite_Buffer *buf, int64_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_uint64(Bakelite_Buffer *buf, uint64_t val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_float32(Bakelite_Buffer *buf, float val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

static inline int bakelite_write_float64(Bakelite_Buffer *buf, double val) {
    return bakelite_buffer_write(buf, &val, sizeof(val));
}

/* Write fixed-size bytes */
static inline int bakelite_write_bytes_fixed(Bakelite_Buffer *buf, const uint8_t *val, uint32_t size) {
    return bakelite_buffer_write(buf, val, size);
}

/* Write variable-length bytes (with length prefix) */
static inline int bakelite_write_bytes(Bakelite_Buffer *buf, const Bakelite_SizedArray *val) {
    int rcode = bakelite_write_uint8(buf, val->size);
    if (rcode != BAKELITE_OK) return rcode;
    return bakelite_buffer_write(buf, val->data, val->size);
}

/* Write fixed-size string */
static inline int bakelite_write_string_fixed(Bakelite_Buffer *buf, const char *val, uint32_t size) {
    return bakelite_buffer_write(buf, val, size);
}

/* Write null-terminated string */
static inline int bakelite_write_string(Bakelite_Buffer *buf, const char *val) {
    if (val == NULL) {
        return bakelite_write_uint8(buf, 0);
    }
    uint32_t len = (uint32_t)strlen(val);
    int rcode = bakelite_buffer_write(buf, val, len);
    if (rcode != BAKELITE_OK) return rcode;
    return bakelite_write_uint8(buf, 0);
}

/* Read primitives */
static inline int bakelite_read_bool(Bakelite_Buffer *buf, bool *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_int8(Bakelite_Buffer *buf, int8_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_uint8(Bakelite_Buffer *buf, uint8_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_int16(Bakelite_Buffer *buf, int16_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_uint16(Bakelite_Buffer *buf, uint16_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_int32(Bakelite_Buffer *buf, int32_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_uint32(Bakelite_Buffer *buf, uint32_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_int64(Bakelite_Buffer *buf, int64_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_uint64(Bakelite_Buffer *buf, uint64_t *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_float32(Bakelite_Buffer *buf, float *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

static inline int bakelite_read_float64(Bakelite_Buffer *buf, double *val) {
    return bakelite_buffer_read(buf, val, sizeof(*val));
}

/* Read fixed-size bytes */
static inline int bakelite_read_bytes_fixed(Bakelite_Buffer *buf, uint8_t *val, uint32_t size) {
    return bakelite_buffer_read(buf, val, size);
}

/* Read variable-length bytes (with length prefix, allocates from heap) */
static inline int bakelite_read_bytes(Bakelite_Buffer *buf, Bakelite_SizedArray *val) {
    uint8_t size;
    int rcode = bakelite_read_uint8(buf, &size);
    if (rcode != BAKELITE_OK) return rcode;

    val->data = bakelite_buffer_alloc(buf, size);
    val->size = size;

    if (val->data == NULL) {
        return BAKELITE_ERR_ALLOC_BYTES;
    }

    return bakelite_buffer_read(buf, val->data, size);
}

/* Read fixed-size string */
static inline int bakelite_read_string_fixed(Bakelite_Buffer *buf, char *val, uint32_t size) {
    return bakelite_buffer_read(buf, val, size);
}

/* Read null-terminated string (allocates from heap) */
static inline int bakelite_read_string(Bakelite_Buffer *buf, char **val) {
    char *start = (char *)bakelite_buffer_alloc(buf, 1);
    *val = start;

    if (start == NULL) {
        return BAKELITE_ERR_ALLOC_STRING;
    }

    char *current = start;
    for (;;) {
        int rcode = bakelite_buffer_read(buf, current, 1);
        if (rcode != BAKELITE_OK) return rcode;

        if (*current == '\0') {
            return BAKELITE_OK;
        }

        current = (char *)bakelite_buffer_alloc(buf, 1);
        if (current == NULL) {
            return BAKELITE_ERR_ALLOC_STRING;
        }
    }
}

/* Write array of primitives (fixed size) */
#define BAKELITE_DEFINE_WRITE_ARRAY(type, name) \
    static inline int bakelite_write_array_##name(Bakelite_Buffer *buf, const type *arr, uint32_t count) { \
        for (uint32_t i = 0; i < count; i++) { \
            int rcode = bakelite_write_##name(buf, arr[i]); \
            if (rcode != BAKELITE_OK) return rcode; \
        } \
        return BAKELITE_OK; \
    }

BAKELITE_DEFINE_WRITE_ARRAY(bool, bool)
BAKELITE_DEFINE_WRITE_ARRAY(int8_t, int8)
BAKELITE_DEFINE_WRITE_ARRAY(uint8_t, uint8)
BAKELITE_DEFINE_WRITE_ARRAY(int16_t, int16)
BAKELITE_DEFINE_WRITE_ARRAY(uint16_t, uint16)
BAKELITE_DEFINE_WRITE_ARRAY(int32_t, int32)
BAKELITE_DEFINE_WRITE_ARRAY(uint32_t, uint32)
BAKELITE_DEFINE_WRITE_ARRAY(int64_t, int64)
BAKELITE_DEFINE_WRITE_ARRAY(uint64_t, uint64)
BAKELITE_DEFINE_WRITE_ARRAY(float, float32)
BAKELITE_DEFINE_WRITE_ARRAY(double, float64)

/* Read array of primitives (fixed size) */
#define BAKELITE_DEFINE_READ_ARRAY(type, name) \
    static inline int bakelite_read_array_##name(Bakelite_Buffer *buf, type *arr, uint32_t count) { \
        for (uint32_t i = 0; i < count; i++) { \
            int rcode = bakelite_read_##name(buf, &arr[i]); \
            if (rcode != BAKELITE_OK) return rcode; \
        } \
        return BAKELITE_OK; \
    }

BAKELITE_DEFINE_READ_ARRAY(bool, bool)
BAKELITE_DEFINE_READ_ARRAY(int8_t, int8)
BAKELITE_DEFINE_READ_ARRAY(uint8_t, uint8)
BAKELITE_DEFINE_READ_ARRAY(int16_t, int16)
BAKELITE_DEFINE_READ_ARRAY(uint16_t, uint16)
BAKELITE_DEFINE_READ_ARRAY(int32_t, int32)
BAKELITE_DEFINE_READ_ARRAY(uint32_t, uint32)
BAKELITE_DEFINE_READ_ARRAY(int64_t, int64)
BAKELITE_DEFINE_READ_ARRAY(uint64_t, uint64)
BAKELITE_DEFINE_READ_ARRAY(float, float32)
BAKELITE_DEFINE_READ_ARRAY(double, float64)
