// Pre-declarations of COBS functions
struct cobs_encode_result;
struct cobs_decode_result
{
    size_t              out_len;
    int                 status;
};
static cobs_encode_result cobs_encode(uint8_t *dst_buf_ptr, size_t dst_buf_len,
                                const uint8_t *src_ptr, size_t src_len);
static cobs_decode_result cobs_decode(uint8_t *dst_buf_ptr, size_t dst_buf_len,
                                const uint8_t *src_ptr, size_t src_len);