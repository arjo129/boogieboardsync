# Boogieboard Sync Driver

This is a boogieboard sync driver for the linux userspace written in C.

## Dependencies
The following packages can be acquired in you distribution's package repository or built from source:

* libusb-1.0
* libevdev

Additionally you need pkg-config and make to build the program.

## Building

For ubuntu one can get the dependencies like so:

	sudo apt-get install make gcc pkg-config libusb-1.0-dev libevdev

Thenk all one needs to do is run make in the directory:

	make

This should build the program. If you recieve linker errors please make sure that you have `libusb-1.0` and not `libusb`.

## Resources
In its current form this is a direct port to c of the driver by [jbedo](https://github.com/jbedo/boogiesync-tablet)

## TODO (maybe)

* support multiple boogieboards
* migrate to async API
* create pretty GUI
* asyncify
