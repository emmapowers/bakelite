/*
 * ctiny COBS Framer - uses common C99 COBS implementation
 */

/* Calculate total buffer size needed for framer */
#define BAKELITE_FRAMER_BUFFER_SIZE(MAX_MSG_SIZE, CRC_SIZE) \
    (BAKELITE_COBS_OVERHEAD((MAX_MSG_SIZE) + (CRC_SIZE)) + (MAX_MSG_SIZE) + (CRC_SIZE) + 1)

/* Message offset in buffer (after COBS overhead) */
#define BAKELITE_FRAMER_MESSAGE_OFFSET(MAX_MSG_SIZE, CRC_SIZE) \
    BAKELITE_COBS_OVERHEAD((MAX_MSG_SIZE) + (CRC_SIZE))

/* Decode state enum */
typedef enum {
    BAKELITE_DECODE_OK = 0,
    BAKELITE_DECODE_NOT_READY,
    BAKELITE_DECODE_FAILURE,
    BAKELITE_DECODE_CRC_FAILURE,
    BAKELITE_DECODE_BUFFER_OVERRUN
} Bakelite_DecodeState;

/* Framer result */
typedef struct {
    int status;
    size_t length;
    uint8_t *data;
} Bakelite_FramerResult;

/* Decode result */
typedef struct {
    Bakelite_DecodeState status;
    size_t length;
    uint8_t *data;
} Bakelite_DecodeResult;

/* CRC type enum */
typedef enum {
    BAKELITE_CRC_NONE = 0,
    BAKELITE_CRC_8,
    BAKELITE_CRC_16,
    BAKELITE_CRC_32
} Bakelite_CrcType;

/* COBS Framer structure */
typedef struct {
    uint8_t *buffer;
    size_t buffer_size;
    size_t max_message_size;
    size_t message_offset;
    size_t crc_size;
    Bakelite_CrcType crc_type;
    uint8_t *read_pos;
} Bakelite_CobsFramer;

/* Get CRC size for a CRC type */
static inline size_t bakelite_crc_size(Bakelite_CrcType crc_type) {
    switch (crc_type) {
        case BAKELITE_CRC_8:  return 1;
        case BAKELITE_CRC_16: return 2;
        case BAKELITE_CRC_32: return 4;
        default: return 0;
    }
}

/* Initialize framer with user-provided buffer */
static inline void bakelite_framer_init(Bakelite_CobsFramer *framer,
                                         uint8_t *buffer, size_t buffer_size,
                                         size_t max_message_size,
                                         Bakelite_CrcType crc_type) {
    framer->buffer = buffer;
    framer->buffer_size = buffer_size;
    framer->max_message_size = max_message_size;
    framer->crc_type = crc_type;
    framer->crc_size = bakelite_crc_size(crc_type);
    framer->message_offset = BAKELITE_COBS_OVERHEAD(max_message_size + framer->crc_size);
    framer->read_pos = buffer;
}

/* Get pointer to message area in buffer */
static inline uint8_t *bakelite_framer_buffer(Bakelite_CobsFramer *framer) {
    return framer->buffer + framer->message_offset;
}

/* Get usable buffer size for messages */
static inline size_t bakelite_framer_buffer_size(Bakelite_CobsFramer *framer) {
    return framer->max_message_size + 1;  /* +1 for type byte */
}

/* Calculate and append CRC to data */
static inline void bakelite_framer_append_crc(Bakelite_CobsFramer *framer, uint8_t *data, size_t length) {
    switch (framer->crc_type) {
        case BAKELITE_CRC_8: {
            uint8_t crc = bakelite_crc8(data, length, 0);
            memcpy(data + length, &crc, sizeof(crc));
            break;
        }
        case BAKELITE_CRC_16: {
            uint16_t crc = bakelite_crc16(data, length, 0);
            memcpy(data + length, &crc, sizeof(crc));
            break;
        }
        case BAKELITE_CRC_32: {
            uint32_t crc = bakelite_crc32(data, length, 0);
            memcpy(data + length, &crc, sizeof(crc));
            break;
        }
        default:
            break;
    }
}

/* Verify CRC of data */
static inline bool bakelite_framer_verify_crc(Bakelite_CobsFramer *framer, const uint8_t *data, size_t length) {
    switch (framer->crc_type) {
        case BAKELITE_CRC_8: {
            uint8_t expected;
            memcpy(&expected, data + length, sizeof(expected));
            return bakelite_crc8(data, length, 0) == expected;
        }
        case BAKELITE_CRC_16: {
            uint16_t expected;
            memcpy(&expected, data + length, sizeof(expected));
            return bakelite_crc16(data, length, 0) == expected;
        }
        case BAKELITE_CRC_32: {
            uint32_t expected;
            memcpy(&expected, data + length, sizeof(expected));
            return bakelite_crc32(data, length, 0) == expected;
        }
        default:
            return true;
    }
}

/* Encode frame with data copy */
static inline Bakelite_FramerResult bakelite_framer_encode_copy(Bakelite_CobsFramer *framer,
                                                                  const uint8_t *data, size_t length) {
    uint8_t *msg_start = framer->buffer + framer->message_offset;
    memcpy(msg_start, data, length);

    if (framer->crc_size > 0) {
        bakelite_framer_append_crc(framer, msg_start, length);
    }

    Bakelite_CobsEncodeResult result = bakelite_cobs_encode(
        framer->buffer, framer->buffer_size,
        msg_start, length + framer->crc_size);

    if (result.status != 0) {
        return (Bakelite_FramerResult){ 1, 0, NULL };
    }

    framer->buffer[result.out_len] = 0;  /* Null terminator */
    return (Bakelite_FramerResult){ 0, result.out_len + 1, framer->buffer };
}

/* Encode frame (data already in buffer at message offset) */
static inline Bakelite_FramerResult bakelite_framer_encode(Bakelite_CobsFramer *framer, size_t length) {
    uint8_t *msg_start = framer->buffer + framer->message_offset;

    if (framer->crc_size > 0) {
        bakelite_framer_append_crc(framer, msg_start, length);
    }

    Bakelite_CobsEncodeResult result = bakelite_cobs_encode(
        framer->buffer, framer->buffer_size,
        msg_start, length + framer->crc_size);

    if (result.status != 0) {
        return (Bakelite_FramerResult){ 1, 0, NULL };
    }

    framer->buffer[result.out_len] = 0;  /* Null terminator */
    return (Bakelite_FramerResult){ 0, result.out_len + 1, framer->buffer };
}

/* Decode a complete frame */
static inline Bakelite_DecodeResult bakelite_framer_decode_frame(Bakelite_CobsFramer *framer, size_t length) {
    if (length == 1) {
        return (Bakelite_DecodeResult){ BAKELITE_DECODE_FAILURE, 0, NULL };
    }

    length--;  /* Discard null byte */

    /* Decode in-place at buffer start */
    Bakelite_CobsDecodeResult result = bakelite_cobs_decode(
        framer->buffer, framer->buffer_size,
        framer->buffer, length);

    if (result.status != 0) {
        return (Bakelite_DecodeResult){ BAKELITE_DECODE_FAILURE, 0, NULL };
    }

    /* Length of decoded data without CRC */
    length = result.out_len - framer->crc_size;

    if (framer->crc_size > 0) {
        if (!bakelite_framer_verify_crc(framer, framer->buffer, length)) {
            return (Bakelite_DecodeResult){ BAKELITE_DECODE_CRC_FAILURE, 0, NULL };
        }
    }

    /* Move decoded data to message offset position for consistent buffer layout */
    if (framer->message_offset > 0) {
        memmove(framer->buffer + framer->message_offset, framer->buffer, length);
    }

    return (Bakelite_DecodeResult){
        BAKELITE_DECODE_OK,
        length,
        framer->buffer + framer->message_offset
    };
}

/* Process a single byte from the stream */
static inline Bakelite_DecodeResult bakelite_framer_read_byte(Bakelite_CobsFramer *framer, uint8_t byte) {
    *framer->read_pos = byte;
    size_t length = (size_t)(framer->read_pos - framer->buffer) + 1;

    if (byte == 0) {
        framer->read_pos = framer->buffer;
        return bakelite_framer_decode_frame(framer, length);
    } else if (length == framer->buffer_size) {
        framer->read_pos = framer->buffer;
        return (Bakelite_DecodeResult){ BAKELITE_DECODE_BUFFER_OVERRUN, 0, NULL };
    }

    framer->read_pos++;
    return (Bakelite_DecodeResult){ BAKELITE_DECODE_NOT_READY, 0, NULL };
}
