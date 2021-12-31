#include "proto.h"

using namespace Bakelite;

int code = 0;

Protocol proto(
    []() { return Serial.read(); },
    [](const char *data, size_t length) { return Serial.write(data, length); }
  );

void setup() {
  Serial.begin(9600);

  while(!Serial) {
    
  }  

  Ack ack;
  ack.code = 42;
  strcpy(ack.message, "Hello world!");
  proto.sendAck(ack);
}

void send_err(uint8_t code, const char *msg) {
  Ack ack;
  ack.code = code;
  strcpy(ack.message, msg);
  proto.sendAck(ack);
}

void loop() {
  switch(proto.poll()) {
  case Protocol::Message::NoMessage:
    break;
  case Protocol::Message::TestMessage:
    TestMessage msg;
    int ret = proto.decodeTestMessage(msg);
    if(ret != 0) {
      send_err("Unpack Failed", ret);
      return;
    }
  
    // Reply
    Ack ack;
    code++;
    ack.code = code;
    snprintf(ack.message, sizeof(ack.message), "a=%d b=%d status=%s msg='%s'", (int)msg.a, (int)msg.b, msg.status ? "true" : "false", msg.message);
    ret = proto.sendAck(ack);

    if(ret != 0) {
      send_err("Pack failed", ret);
      return;
    }
    break;
  }
}
