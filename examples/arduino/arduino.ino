#include "proto.h"

// Create an instance of our protocol. Use the Serial port for sending and receiving data.
Protocol proto(
  []() { return Serial.read(); },
  [](const char *data, size_t length) { return Serial.write(data, length); }
);

// keep track of how many responses we've set.
int numResponses = 0;

void setup() {
  Serial.begin(9600);

  // For boards that have a native USB port, wait for the serial device to be initialized.
  while(!Serial) {}

  // Send a hello message, because, why not?
  Ack ack;
  ack.code = 42;
  strcpy(ack.message, "Hello world!");
  proto.send(ack);
}

// Send an error message to the PC for debugging.
void send_err(const char *msg, uint8_t code) {
  Ack ack;
  ack.code = code;
  strcpy(ack.message, msg);
  proto.send(ack);
}

void loop() {
  // Check and see if a new message has arrived
  Protocol::Message messageId = proto.poll();
  
  switch(messageId) {
  case Protocol::Message::NoMessage: // Nope, better luch next time
    break;

  case Protocol::Message::TestMessage: // We received a test message!
  {
    //Decode the test message
    TestMessage msg;
    int ret = proto.decode(msg);
    if(ret != 0) {
      send_err("Decode Failed", ret);
      return;
    }
  
    // Reply
    numResponses++;
    
    Ack ack;
    ack.code = numResponses;
    snprintf(ack.message, sizeof(ack.message), "a=%d b=%d status=%s msg='%s'", (int)msg.a, (int)msg.b, msg.status ? "true" : "false", msg.message);
    ret = proto.send(ack);

    if(ret != 0) {
      send_err("Send failed", ret);
      return;
    }
    break;
  }
  default:  // Just in case we get something unexpected...
    send_err("Unkown message received!", (uint8_t)messageId);
    break;
  }
}
