# Talking to an Arduino from Python
A simple example showing how to communicate with an Arduino from a Python script running on a PC. It demonstrates a very simple protocol, and how to use the protocol to talk to an Arduino over a serial port.

__Requirements:__
* Any Arduino board. (Tested with Uno, Nano, Leonardo)
* [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/)
* Python 3.8+
* make (optional)

### Setup and Usage

Install bakelite and pyserial:
```bash
pip3 install bakelite pyserial
```

The required source files have been pre-generated for this project.
If you'd like to make changes to [proto.bakelite](./proto.bakelite), run:
```bash
make
```

### Building and Uploading

**With PlatformIO:**
```bash
make build                    # Build for all boards
pio run -e uno -t upload      # Upload to Arduino Uno
```

**With Arduino IDE:**
Open [arduino.ino](./arduino.ino) and use the IDE to compile and upload.

### Testing

Find the com port (Windows) or device (Linux/Mac) your board is connected to and run:
```bash
python3 ./test.py [port/device]
```