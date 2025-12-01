/* Buffer stream for serialization/deserialization */
typedef struct {
    uint8_t *buffer;
    uint32_t size;
    uint32_t pos;
} Bakelite_Buffer;

/* Initialize a buffer stream */
static inline void bakelite_buffer_init(Bakelite_Buffer *buf, uint8_t *data, uint32_t size) {
    buf->buffer = data;
    buf->size = size;
    buf->pos = 0;
}

/* Reset buffer position to start */
static inline void bakelite_buffer_reset(Bakelite_Buffer *buf) {
    buf->pos = 0;
}

/* Write data to buffer */
static inline int bakelite_buffer_write(Bakelite_Buffer *buf, const void *data, uint32_t length) {
    uint32_t end_pos = buf->pos + length;
    if (end_pos > buf->size) {
        return BAKELITE_ERR_WRITE;
    }
    memcpy(buf->buffer + buf->pos, data, length);
    buf->pos += length;
    return BAKELITE_OK;
}

/* Read data from buffer */
static inline int bakelite_buffer_read(Bakelite_Buffer *buf, void *data, uint32_t length) {
    uint32_t end_pos = buf->pos + length;
    if (end_pos > buf->size) {
        return BAKELITE_ERR_READ;
    }
    memcpy(data, buf->buffer + buf->pos, length);
    buf->pos += length;
    return BAKELITE_OK;
}

/* Seek to position */
static inline int bakelite_buffer_seek(Bakelite_Buffer *buf, uint32_t pos) {
    if (pos >= buf->size) {
        return BAKELITE_ERR_SEEK;
    }
    buf->pos = pos;
    return BAKELITE_OK;
}

/* Get current position */
static inline uint32_t bakelite_buffer_pos(const Bakelite_Buffer *buf) {
    return buf->pos;
}

/* Get buffer size */
static inline uint32_t bakelite_buffer_size(const Bakelite_Buffer *buf) {
    return buf->size;
}

/* Get remaining bytes */
static inline uint32_t bakelite_buffer_remaining(const Bakelite_Buffer *buf) {
    return buf->size - buf->pos;
}
