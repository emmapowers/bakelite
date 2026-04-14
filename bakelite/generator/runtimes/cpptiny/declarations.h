// C++ wrapper types and functions for COBS encode/decode
// These wrap the common C99 implementation for use in the Bakelite namespace

struct cobs_encode_result {
    size_t out_len;
    int status;
};

struct cobs_decode_result {
    size_t out_len;
    int status;
};

static inline cobs_encode_result cobs_encode(void *dst_buf_ptr, size_t dst_buf_len,
                                             const void *src_ptr, size_t src_len) {
    Bakelite_CobsEncodeResult c_result = bakelite_cobs_encode(dst_buf_ptr, dst_buf_len, src_ptr, src_len);
    return {c_result.out_len, c_result.status};
}

static inline cobs_decode_result cobs_decode(void *dst_buf_ptr, size_t dst_buf_len,
                                             const void *src_ptr, size_t src_len) {
    Bakelite_CobsDecodeResult c_result = bakelite_cobs_decode(dst_buf_ptr, dst_buf_len, src_ptr, src_len);
    return {c_result.out_len, c_result.status};
}
