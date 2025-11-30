#include "proto.h"

// Create an instance of our protocol. Use the Serial port for sending and receiving data.
Protocol proto(
  []() { return Serial.read(); },
  [](const char *data, size_t length) { return Serial.write(data, length); }
);

// keep track of how many responses we've sent.
int numResponses = 0;

void setup() {
  Serial.begin(9600);

  // For boards that have a native USB port, wait for the serial device to be initialized.
  while(!Serial) {}

  // Send a hello message using zero-copy API
  auto& ack = proto.message<Ack>();
  ack.code = 42;
  strcpy(ack.message, "Hello world!");
  proto.send<Ack>();
}

// Send an error message to the PC for debugging.
void send_err(const char *msg, uint8_t code) {
  auto& ack = proto.message<Ack>();
  ack.code = code;
  strcpy(ack.message, msg);
  proto.send<Ack>();
}

void loop() {
  // Check and see if a new message has arrived
  Protocol::Message messageId = proto.poll();

  switch(messageId) {
  case Protocol::Message::NoMessage: // Nope, better luck next time
    break;

  case Protocol::Message::TestMessage: // We received a test message!
  {
    // Access the message directly in the buffer (zero-copy)
    const auto& msg = proto.message<TestMessage>();

    // Copy data we need before preparing the response
    // (response will overwrite the buffer)
    uint8_t a = msg.a;
    int32_t b = msg.b;
    bool status = msg.status;
    char msgText[16];
    strncpy(msgText, msg.message, sizeof(msgText));

    // Reply
    numResponses++;

    auto& ack = proto.message<Ack>();
    ack.code = numResponses;
    snprintf(ack.message, sizeof(ack.message), "a=%d b=%d status=%s msg='%s'",
             (int)a, (int)b, status ? "true" : "false", msgText);

    int ret = proto.send<Ack>();
    if(ret != 0) {
      send_err("Send failed", ret);
      return;
    }
    break;
  }
  default:  // Just in case we get something unexpected...
    send_err("Unknown message received!", (uint8_t)messageId);
    break;
  }
}
