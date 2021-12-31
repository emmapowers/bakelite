# Talking to an Arduino from Python
A simple example showing how to communicate with an Arduino from a Python script running on a PC. It demonstrates a very simple protocol, and how to use the protocol to talk to an Arduino over a serial port.

__Requirements:__
* Any Arduino board. (Tested with Uno, Nano, Leonardo)
* The [Arduino IDE](https://www.arduino.cc/en/software)
* Python 3.8+
* make (optional)

### Setup and Usage
Download and install the [Arduino IDE](https://www.arduino.cc/en/software).
Open the [arduino.ino](./arduino.ino) sketch in this folder.


Install bakelite and pyserial.
```bash
$ pip3 install bakelite pyserial
```

The required source files have been pre-generated for this project.
If you'd like to make changes to [proto.bakelite](./proto.bakelite), run:
```bash
$ make
```

Next, use the Arduino IDE to compile and upload the code to your Arduino board.
Find the com port (Windows) or device (Linux/Mac) your board is connected to and run:
```bash
python3 ./test.py [port/device]
```