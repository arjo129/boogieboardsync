CC=gcc
CFLAGS=`pkg-config --cflags libusb-1.0 libevdev`
LIBS=`pkg-config --libs libusb-1.0 libevdev`
SRC=$(wildcard *.c)
driver: $(SRC)
	gcc -g $^ $(CFLAGS) $(LIBS) -o$@
