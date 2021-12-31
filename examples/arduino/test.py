import sys
from proto import Protocol, TestMessage, Ack
from bakelite.proto import Framer
from io import BytesIO
import serial

if(len(sys.argv) != 2):
  print("Usage: test.py [serial port]")
  sys.exit(1)

with serial.Serial(sys.argv[1], 9600) as port:
  framer = Framer(crc=None)

  msg = TestMessage(
    a=123,
    b=-124,
    status=True,
    message="Ping!".encode('utf-8')
  )
  stream = BytesIO()
  msg.pack(stream)
  f = framer.encode_frame(stream.getvalue())
  port.write(f)

  while True:
    data = port.read()
    framer.append_buffer(data)
    frame = framer.decode_frame()
    if(frame):
      ack = Ack.unpack(BytesIO(frame))
      print(f"Ack code={ack.code} message=\"{ack.message.decode('utf-8')}\"")