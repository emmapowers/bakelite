import sys
from proto import Protocol, TestMessage, Ack
from io import BytesIO
import serial

if(len(sys.argv) != 2):
  print("Usage: test.py [serial port]")
  sys.exit(1)

with serial.Serial(sys.argv[1], 9600) as port:
  proto = Protocol(stream=port)

  msg = TestMessage(
    a=123,
    b=-124,
    status=True,
    message="Ping!".encode('utf-8')
  )
  proto.send(msg)

  while True:
    msg = proto.poll()
    if msg:
      if isinstance(msg, Ack):
        print(f"Ack code={msg.code} message=\"{msg.message.decode('utf-8')}\"")
      else:
        print(msg)