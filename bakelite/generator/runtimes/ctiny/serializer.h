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

/* Write variable-length bytes (length prefix + data) */
static inline int bakelite_write_bytes(Bakelite_Buffer *buf, const uint8_t *data, uint8_t len) {
    int rcode = bakelite_write_uint8(buf, len);
    if (rcode != BAKELITE_OK) return rcode;
    return bakelite_buffer_write(buf, data, len);
}

/* Write null-terminated string */
static inline int bakelite_write_string(Bakelite_Buffer *buf, const char *val) {
    uint32_t len = (uint32_t)strlen(val);
    int rcode = bakelite_buffer_write(buf, val, len);
    if (rcode != BAKELITE_OK) return rcode;
    return bakelite_write_uint8(buf, 0);  /* null terminator */
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

/* Read variable-length bytes into inline storage (length prefix + data) */
static inline int bakelite_read_bytes(Bakelite_Buffer *buf, uint8_t *data, uint8_t *len, uint8_t capacity) {
    uint8_t size;
    int rcode = bakelite_read_uint8(buf, &size);
    if (rcode != BAKELITE_OK) return rcode;

    if (size > capacity) {
        return BAKELITE_ERR_CAPACITY;
    }

    *len = size;
    return bakelite_buffer_read(buf, data, size);
}

/* Read null-terminated string into char array */
static inline int bakelite_read_string(Bakelite_Buffer *buf, char *val, uint32_t capacity) {
    uint32_t i = 0;
    while (i < capacity - 1) {
        int rcode = bakelite_buffer_read(buf, &val[i], 1);
        if (rcode != BAKELITE_OK) return rcode;
        if (val[i] == '\0') {
            return BAKELITE_OK;
        }
        i++;
    }
    /* Read and discard until null terminator or error */
    char c;
    do {
        int rcode = bakelite_buffer_read(buf, &c, 1);
        if (rcode != BAKELITE_OK) return rcode;
    } while (c != '\0');
    val[capacity - 1] = '\0';
    return BAKELITE_OK;
}

/* Write array of primitives */
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

/* Read array of primitives */
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
