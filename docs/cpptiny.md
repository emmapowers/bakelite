# C++ (Tiny)
The `cpptiny` target is designed to be used in embedded systems with highly constrained resources.
It has a small memory footprint and does not allocate on the heap or use the STL.
It works well on microcontrollers with under 2k of ram, or on a desktop PC.
A compiler that can target the `C++14` standard is required.

## Features
  * Header only
  * Small memory footprint
  * No heap memory allocation
  * Does not use the STL
  * Produces well optimized assembly when compiler optimizations are enabled

## Example
Code generation
```sh
$ bakelite runtime -l cpptiny -o bakelite.h  # One-time generation of the library
$ bakelite gen -l cpptiny -i proto.bakelite -o proto.h
```

Use the generated code to implement a simple protocol.
```c++
#include "proto.h"

int main(int argc, char *arv[]) {
  Serial port("/dev/ttyUSB0", 9600); // Magic, easy to use, portable serial port class.. :)

  // Create an instance of our protocol. Use the Serial port for sending and receiving data.
  Protocol proto(
    []() { return port.read(); },
    [](const char *data, size_t length) { return port.write(data, length); }
  );

  // Send a message
  HelloMsg msg;
  msg.code = 42;
  strcpy(msg.message, "Hello world!");
  proto.send(msg);

  // Wait for a reply
  while(true) {
    // Check and see if a new message has arrived
    Protocol::Message messageId = proto.poll();

    switch(messageId) {
    case Protocol::Message::NoMessage: // Nope, better luch next time
      break;

    case Protocol::Message::ReplyMsg: // We received a reply!
      //Decode message
      ReplyMsg msg;
      int ret = proto.decode(msg);
      if(ret != 0) {
        send_err("Decode Failed", ret);
        return;
      }

      cout << "Reply: " << msg.text << endl;
    default:
      send_err("Unkown message id:", messageId);
      break;
  }
}
```

## Memory Management
