import sys
from proto import Protocol, TestMessage, Ack
from io import BytesIO
import serial

# Get the port name from the command line arguments
if(len(sys.argv) != 2):
  print("Usage: test.py [serial port]")
  sys.exit(1)
port_name = sys.argv[1]

# Open the serial port at 9600 baud
with serial.Serial(port_name, 9600) as port:
  # Create an instance of our protocol, passing the port
  # we just opened as the stream.
  proto = Protocol(stream=port)

  # Send a test message
  msg = TestMessage(
    a=123,
    b=-124,
    status=True,
    message="Ping!".encode('utf-8')
  )
  proto.send(msg)

  # Wait for responses, and print them.
  while True:
    # Poll for new messages. By default, this function blocks
    # untill there is data to read.
    msg = proto.poll()
    if msg:
      if isinstance(msg, Ack): # if we got an ack, print it
        print(f"Ack code={msg.code} message=\"{msg.message.decode('utf-8')}\"")
      else: # We shouldn't get anything else, but just in case, print it too
        print(msg)