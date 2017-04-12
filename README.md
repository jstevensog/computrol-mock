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
computrol-mock is an executable binary, currently with no options.  When you run it, it will report what it is doing to the console.
You will see inbound and outbound modbus messages, and any issues with them.  Inbound modbus messages are displayed in hex strings, with each octet surrounded by <>.  Output hex strings are displayed with each octet surrounded by [].
Note, it MUST be run as root.

```
sudo computrol-mock
```

to run in the foreground, or if you wish to run it in the background to reduce the number of open terminals to the pi,  

```
sudo compurol-mock &
```

As it was initially used to test the protocol in the LFI app for GT, every time a request is received from the GT board, holding registers >13 are incremented.

A future version will allow an interface to enable named registers to be set (such as Voltage, Current, Temp, etc), and all registers to be queried.

You can also simulate a "report by exception" alarm by using a command similar to that below.

```
sudo pkill -SIGRTMIN+1 computrol-mock
```

The program will set register 3 alarm bits based on the SIGRTMIN+x value.
SIGRTMIN+1 - Level 3 Current Alarm, 001 in bits 1-3 (note bit 0 is ignored).
SIGRTMIN+2 - Level 2 Current Alarm, 010 in bits 1-3 (note bit 0 is ignored).
SIGRTMIN+3 - Level 1 Current Alarm, 100 in bits 1-3 (note bit 0 is ignored).
SIGRTMIN+4 - Voltage Low Alarm, Bit 4.
SIGRTMIN+5 - Voltage Normal Alarm, Bit 5.

This will cause computrol-mock to assert the WAKEUP pin on the GT board, and it will wait for the AWAKE pin to be asserted before transmitting.

You can stop computrol-mock using Ctrl-C in a console if you are running it in the foreground, or sudo pkill computrol-mock if it is running in the background.

That is it for now.  Any requests for additional functionality or changes, get in touch, or do it yourself, and remember to push up to the repo. ;)

