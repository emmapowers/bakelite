#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "bakelite.h"

/* Platform check for packed struct support (unaligned access required) */
#if defined(__AVR__) || (defined(__ARM_ARCH) && __ARM_ARCH >= 7) || \
    defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
  #define BAKELITE_UNALIGNED_OK 1
#else
  #define BAKELITE_UNALIGNED_OK 0
#endif

BAKELITE_STATIC_ASSERT(BAKELITE_UNALIGNED_OK, platform_requires_unpacked_mode);
/* Forward declarations */
struct ChatMessage;
struct SetName;
/* Enums */
/* Structs */
typedef struct BAKELITE_PACKED {
  char sender[33];
  char text[257];
} ChatMessage;

static inline int ChatMessage_pack(const ChatMessage *self, Bakelite_Buffer *buf) {
  int rcode = 0;
  if ((rcode = bakelite_write_string(buf, self->sender)) != 0) return rcode;
  if ((rcode = bakelite_write_string(buf, self->text)) != 0) return rcode;
  return rcode;
}

static inline int ChatMessage_unpack(ChatMessage *self, Bakelite_Buffer *buf) {
  int rcode = 0;
  if ((rcode = bakelite_read_string(buf, self->sender, 33)) != 0) return rcode;
  if ((rcode = bakelite_read_string(buf, self->text, 257)) != 0) return rcode;
  return rcode;
}

typedef struct BAKELITE_PACKED {
  char name[33];
} SetName;

static inline int SetName_pack(const SetName *self, Bakelite_Buffer *buf) {
  int rcode = 0;
  if ((rcode = bakelite_write_string(buf, self->name)) != 0) return rcode;
  return rcode;
}

static inline int SetName_unpack(SetName *self, Bakelite_Buffer *buf) {
  int rcode = 0;
  if ((rcode = bakelite_read_string(buf, self->name, 33)) != 0) return rcode;
  return rcode;
}

/* Protocol message IDs */
typedef enum {
  Protocol_NoMessage = -1,
  Protocol_ChatMessage = 1,
  Protocol_SetName = 2,
} Protocol_Message;

/* Protocol buffer sizes */
#define PROTOCOL_MAX_MESSAGE_SIZE 290
#define PROTOCOL_CRC_SIZE 0
#define PROTOCOL_BUFFER_SIZE BAKELITE_FRAMER_BUFFER_SIZE(PROTOCOL_MAX_MESSAGE_SIZE, PROTOCOL_CRC_SIZE)
#define PROTOCOL_MESSAGE_OFFSET BAKELITE_FRAMER_MESSAGE_OFFSET(PROTOCOL_MAX_MESSAGE_SIZE, PROTOCOL_CRC_SIZE)

/* Protocol handler */
typedef struct {
  int (*read_byte)(void);
  size_t (*write)(const uint8_t *data, size_t length);
  Bakelite_CobsFramer framer;
  uint8_t buffer[PROTOCOL_BUFFER_SIZE];
  Protocol_Message received_message;
  size_t received_frame_length;
} Protocol;

static inline void Protocol_init(Protocol *self,
                                  int (*read_byte)(void),
                                  size_t (*write)(const uint8_t *data, size_t length)) {
  self->read_byte = read_byte;
  self->write = write;
  self->received_message = Protocol_NoMessage;
  self->received_frame_length = 0;
  bakelite_framer_init(&self->framer, self->buffer, PROTOCOL_BUFFER_SIZE,
                       PROTOCOL_MAX_MESSAGE_SIZE, BAKELITE_CRC_NONE);
}

static inline Protocol_Message Protocol_poll(Protocol *self) {
  int byte = self->read_byte();
  if (byte < 0) {
    return Protocol_NoMessage;
  }

  Bakelite_DecodeResult result = bakelite_framer_read_byte(&self->framer, (uint8_t)byte);
  if (result.status == BAKELITE_DECODE_OK) {
    if (result.length == 0) {
      return Protocol_NoMessage;
    }

    self->received_message = (Protocol_Message)result.data[0];
    self->received_frame_length = result.length - 1;
    return self->received_message;
  }

  return Protocol_NoMessage;
}

/* Get pointer to message data in buffer (for zero-copy access) */
static inline uint8_t *Protocol_buffer(Protocol *self) {
  return bakelite_framer_buffer(&self->framer) + 1;
}

/* Zero-copy message access (packed mode only) */
static inline ChatMessage *Protocol_message_ChatMessage(Protocol *self) {
  return (ChatMessage *)(bakelite_framer_buffer(&self->framer) + 1);
}

static inline SetName *Protocol_message_SetName(Protocol *self) {
  return (SetName *)(bakelite_framer_buffer(&self->framer) + 1);
}

/* Zero-copy send functions (packed mode only) */
static inline int Protocol_send_zerocopy_ChatMessage(Protocol *self) {
  bakelite_framer_buffer(&self->framer)[0] = (uint8_t)Protocol_ChatMessage;
  size_t frame_size = sizeof(ChatMessage) + 1;
  Bakelite_FramerResult result = bakelite_framer_encode(&self->framer, frame_size);

  if (result.status != 0) {
    return result.status;
  }

  size_t ret = self->write(result.data, result.length);
  return (ret == result.length) ? 0 : -1;
}

static inline int Protocol_send_zerocopy_SetName(Protocol *self) {
  bakelite_framer_buffer(&self->framer)[0] = (uint8_t)Protocol_SetName;
  size_t frame_size = sizeof(SetName) + 1;
  Bakelite_FramerResult result = bakelite_framer_encode(&self->framer, frame_size);

  if (result.status != 0) {
    return result.status;
  }

  size_t ret = self->write(result.data, result.length);
  return (ret == result.length) ? 0 : -1;
}

/* Copy-based send functions */
static inline int Protocol_send_ChatMessage(Protocol *self, const ChatMessage *msg) {
  uint8_t *msg_buf = bakelite_framer_buffer(&self->framer);
  msg_buf[0] = (uint8_t)Protocol_ChatMessage;

  Bakelite_Buffer buf;
  bakelite_buffer_init(&buf, msg_buf + 1, bakelite_framer_buffer_size(&self->framer) - 1);
  ChatMessage_pack(msg, &buf);

  size_t frame_size = bakelite_buffer_pos(&buf) + 1;
  Bakelite_FramerResult result = bakelite_framer_encode(&self->framer, frame_size);

  if (result.status != 0) {
    return result.status;
  }

  size_t ret = self->write(result.data, result.length);
  return (ret == result.length) ? 0 : -1;
}

static inline int Protocol_send_SetName(Protocol *self, const SetName *msg) {
  uint8_t *msg_buf = bakelite_framer_buffer(&self->framer);
  msg_buf[0] = (uint8_t)Protocol_SetName;

  Bakelite_Buffer buf;
  bakelite_buffer_init(&buf, msg_buf + 1, bakelite_framer_buffer_size(&self->framer) - 1);
  SetName_pack(msg, &buf);

  size_t frame_size = bakelite_buffer_pos(&buf) + 1;
  Bakelite_FramerResult result = bakelite_framer_encode(&self->framer, frame_size);

  if (result.status != 0) {
    return result.status;
  }

  size_t ret = self->write(result.data, result.length);
  return (ret == result.length) ? 0 : -1;
}

/* Copy-based decode functions */
static inline int Protocol_decode_ChatMessage(Protocol *self, ChatMessage *msg) {
  if (self->received_message != Protocol_ChatMessage) {
    return -1;
  }

  Bakelite_Buffer buf;
  bakelite_buffer_init(&buf,
    bakelite_framer_buffer(&self->framer) + 1, self->received_frame_length);
  return ChatMessage_unpack(msg, &buf);
}

static inline int Protocol_decode_SetName(Protocol *self, SetName *msg) {
  if (self->received_message != Protocol_SetName) {
    return -1;
  }

  Bakelite_Buffer buf;
  bakelite_buffer_init(&buf,
    bakelite_framer_buffer(&self->framer) + 1, self->received_frame_length);
  return SetName_unpack(msg, &buf);
}

#endif /* PROTOCOL_H */
