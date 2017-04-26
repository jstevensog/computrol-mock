# Computrol Mock

A small RPI program for emulating the Computrol LFI.
## Prerequisites
### wiringPi - Library and utilities for configuring and working with the RPI GPIO pins.
1. Clone the git repo of wiringPi.

```
git clone git://git.drogon.net/wiringPi
```

1. Once you have it perform 

```
cd wiringPi; make && sudo make install
```

You should then be able to clone the computrol-mock repo, change into the computrol-mock directory, and perform a make.

# Connecting to RPI

computrol-mock uses /dev/ttyAMA0 by default.  Connect a 3.3v USB serial adaptor to the rpi, then connect the serial lines to the GT tx and rx pins (NOT console_tx and console_rx) appropriately to the adaptor.  In another version I will make this configurable from the command line.  For now, this will do.
Also, in another future version I will make a bit bash interface for the GT TX/RX pins on the Test Harness and use that instead.

You should also ensure that the WAKEUP and AWAKE pins on the GT board are connected to the GT Test Harness board correctly.  These will be used in any alarm emulation.

# Working with it.

computrol-mock is an executable binary.
It has the following options:
-D path_to_serial_device option.  
-B baud_rate (default 9600).

Note: the -D option defaults to /dev/ttyUSB0, so if that is the port you have connected to the GT tx/rx pins, you do not need to specify it. The -B option MUST be a valid baudrate, or an error message will be printed.

When you run it, it will report what it is doing to the console.
You will see inbound and outbound modbus messages, and any issues with them.  
Inbound modbus messages are displayed in hex strings, with each octet surrounded by <>.  
Output hex strings are displayed with each octet surrounded by [].

Note, it MUST be run as root.

```
sudo computrol-mock -D /dev/ttyUSB1
```

to run in the foreground, or if you wish to run it in the background to reduce the number of open terminals to the pi,  

```
sudo compurol-mock -D /dev/ttyUSB1 &
```

NOTE: Runing in the foreground will prevent computrol-mock receiving register commands (see below) from the console.  I will be soon adding a file socket to allow commands to be sent when computrol-mock is running in the background.

As it was initially used to test the protocol in the LFI app for GT, every time a request is received from the GT board, holding registers >13 are incremented.

A future version will allow an interface to enable named registers to be set (such as Voltage, Current, Temp, etc), and all registers to be queried.

You can also simulate a "report by exception" alarm by using a command similar to that below.

```
sudo pkill -SIGRTMIN+1 computrol-mock
```

The program will set register 5 alarm bits based on the SIGRTMIN+x value.
SIGRTMIN+1 - Level 3 Current Alarm, 001 in bits 1-3 (note bit 0 is ignored).
SIGRTMIN+2 - Level 2 Current Alarm, 010 in bits 1-3 (note bit 0 is ignored).
SIGRTMIN+3 - Level 1 Current Alarm, 100 in bits 1-3 (note bit 0 is ignored).
SIGRTMIN+4 - Voltage Low Alarm, Bit 4.
SIGRTMIN+5 - Voltage Normal Alarm, Bit 5.

This will cause computrol-mock to assert the WAKEUP pin on the GT board, and it will wait for the AWAKE pin to be asserted before transmitting.

Additionally, the program will receive SIGRTMIN+6.  This will cause the Register 12 value to be reset to 255.  
Use this to save killing and restarting computrol-mock when you wish to test that gt_startup correctly sets this to 0.

You can stop computrol-mock using Ctrl-C in a console if you are running it in the foreground, or sudo pkill computrol-mock if it is running in the background.

# Register Commands
The following Register commands are accepted on the console.

1. R X - where X is the register number. Reads the current register value.
2. W X V - where X is the register number, and V is the value to be set.  Writes the value to the specified register, and displays the result.

That is it for now.  Any requests for additional functionality or changes, get in touch, or do it yourself, and remember to push up to the repo. ;)

