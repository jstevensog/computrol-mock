
CC = gcc
MODBUS = libmodbus/src
CFLAGS = -Wall -g -I/usr/include -Ilibmodbus -I$(MODBUS) -I/usr/local/include -L/usr/local/lib -lwiringPi
CXXFLAGS = $(CFLAGS)

all: modbus.o modbus-rtu.o computrol-mock.o computrol-mock
modbus.o: $(MODBUS)/modbus.c $(MODBUS)/modbus.h
	$(CC) $(CFLAGS) -c $(MODBUS)/modbus.c

modbus-data.o: $(MODBUS)/modbus-data.c
	$(CC) $(CFLAGS) -c $(MODBUS)/modbus-data.c

modbus-rtu.o: $(MODBUS)/modbus-rtu.c $(MODBUS)/modbus-rtu.h $(MODBUS)/modbus-rtu-private.h
	$(CC) $(CFLAGS) -c $(MODBUS)/modbus-rtu.c

computrol-mock: computrol-mock.c modbus-data.o modbus-rtu.o modbus.o
	$(CC) $(CFLAGS) computrol-mock.c -o computrol-mock modbus-rtu.o modbus-data.o modbus.o

clean:
	rm *.o computrol-mock

