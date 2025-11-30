/* Buffer stream for serialization/deserialization */
typedef struct {
    uint8_t *buffer;
    uint32_t size;
    uint32_t pos;
    /* Heap for variable-length data allocations */
    uint8_t *heap;
    uint32_t heap_size;
    uint32_t heap_pos;
} Bakelite_Buffer;

/* Initialize a buffer stream (without heap) */
static inline void bakelite_buffer_init(Bakelite_Buffer *buf, uint8_t *data, uint32_t size) {
    buf->buffer = data;
    buf->size = size;
    buf->pos = 0;
    buf->heap = NULL;
    buf->heap_size = 0;
    buf->heap_pos = 0;
}

/* Initialize a buffer stream with heap for variable-length data */
static inline void bakelite_buffer_init_with_heap(Bakelite_Buffer *buf,
                                                   uint8_t *data, uint32_t size,
                                                   uint8_t *heap, uint32_t heap_size) {
    buf->buffer = data;
    buf->size = size;
    buf->pos = 0;
    buf->heap = heap;
    buf->heap_size = heap_size;
    buf->heap_pos = 0;
}

/* Reset buffer position to start */
static inline void bakelite_buffer_reset(Bakelite_Buffer *buf) {
    buf->pos = 0;
    buf->heap_pos = 0;
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

/* Allocate from heap (for variable-length data during deserialization) */
static inline void *bakelite_buffer_alloc(Bakelite_Buffer *buf, uint32_t bytes) {
    uint32_t new_pos = buf->heap_pos + bytes;
    if (buf->heap == NULL || new_pos > buf->heap_size) {
        return NULL;
    }
    void *data = buf->heap + buf->heap_pos;
    buf->heap_pos = new_pos;
    return data;
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
