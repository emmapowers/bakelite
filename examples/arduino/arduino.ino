#include "proto.h"

using namespace Bakelite;

CobsFramer<CrcNoop, 256> framer;

int code = 0;

void setup() {
  Serial.begin(9600);

  while(!Serial) {
    
  }

  Ack ack;
  ack.code = 42;
  strcpy(ack.message, "Hello world!");
  BufferStream outStream((char *)framer.writeBuffer(), framer.writeBufferSize());
  ack.pack(outStream);
  auto encodeResult = framer.encodeFrame(sizeof(ack));
  Serial.write(encodeResult.data, encodeResult.length);
}

void send_err(const char *msg, uint8_t code = 255) {
  Ack ack;
  ack.code = code;
  strcpy(ack.message, msg);
  BufferStream outStream((char *)framer.writeBuffer(), framer.writeBufferSize());
  ack.pack(outStream);
  auto encodeResult = framer.encodeFrame(sizeof(ack));
  Serial.write(encodeResult.data, encodeResult.length);
}

void loop() {
  if(Serial.available() == 0)
    return;
  int byte = Serial.read();
  if(byte >= 0) {
    auto result = framer.readFrameByte((uint8_t)byte);
    if(result.status == CobsDecodeState::Decoded) {
      BufferStream stream((char *)result.data, result.length);
      TestMessage msg;
      int ret = msg.unpack(stream);

      if(ret != 0) {
        send_err("Unpack failed", ret);
        return;
      }

      // Reply
      Ack ack;
      code++;
      ack.code = code;
      snprintf(ack.message, sizeof(ack.message), "a=%d b=%d status=%02x msg='%s'", (int)msg.a, (int)msg.b, msg.status, msg.message);
      BufferStream outStream((char *)framer.writeBuffer(), framer.writeBufferSize());
      ret = ack.pack(outStream);

      if(ret != 0) {
        send_err("Pack failed", ret);
        return;
      }
      
      auto encodeResult = framer.encodeFrame(sizeof(ack));
      Serial.write(encodeResult.data, encodeResult.length);
    }
  }
  
}
