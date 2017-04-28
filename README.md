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

NOTE: Runing in the background will prevent computrol-mock receiving register commands (see below) from the console.  However, you can still send register commands to it using the named pipe method decribed below.

You can stop computrol-mock using Ctrl-C in a console if you are running it in the foreground, or 
```
sudo pkill computrol-mock
```
if it is running in the background.

# Register Commands
The following Register commands are accepted on the console.

| Command | Description |
|---|---|
|R reg | Read Register, where 'reg' is the register number. Reads the current register value.|
|W reg val | Write Register, where 'val' is the register number, and V is the value to be set.  Writes the value to the specified register, and displays the result.|
|I val | Write line Current value, where 'val' is a valid Current value to be written into register 100.|
|V val | Write line Voltage value, where 'val' is a valid Voltage value to be written into register 101.|
|T val | Write line Temperature value, where 'val' is a valid Conductor Temperature (0-100 C) to be written into register 84.|

**Note, there is NO error checking on values.  The range of values that can be written into any register is 0-65535.**

## Sending Register commands using a pipe socket.
When computrol-mock starts, it will create a named pipe node in the current working directory called cm-fifo.  This node as write permissions to everyone, so you can write to it even when running computro-mock under sudo as it expects.

You can create a file of register commands to send to the computrol-mock and cat it to the pipe. Here is an example of sending a file of commands to configure some alarm limits and time values in registers for Alarm Group 1.

```
Contents of init_mock.txt:
W20 700
W21 750
W22 800
W23 850
W60 10000
W61 10000
W62 10000
W63 10000

Command to use:
cat init_mock.txt > cm-fifo

```

Inidividual commands can also be sent using echo.  To set the Line Current value to 710 Amps, 

```
echo I710 >cm-fifo
```

Using this method, complex scenarios can be scripted with bash to perfrom the tests you require.

#Conclusion
That is it for now.  Any requests for additional functionality or changes, get in touch, or do it yourself, and remember to push up to the repo. ;)

