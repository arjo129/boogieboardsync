#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <libusb-1.0/libusb.h>
#include <libevdev/libevdev-uinput.h>
int main(int argc, char** argv) {
	//Initiallize the library
	libusb_device **devs;	
	libusb_init(NULL);
	//libusb_set_debug (NULL, 4);

	//Search for devices
	printf("Searching for device...\n");
	ssize_t cnt = libusb_get_device_list(NULL, &devs);
	
	//Unable to open device return -1;
	if(cnt < 0) {
		libusb_exit(NULL);
		return -1;
	}
	
	//Found boogieboard
	libusb_device *boogieboard = NULL;
	ssize_t i = 0;
	for(i = 0; i < cnt; i++){
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(devs[i],&desc);
		if(desc.idProduct == 0x100 && desc.idVendor == 0x2914){
			printf("BoogieBoard Found\n");
			boogieboard = devs[i];
		}
	}
	libusb_device_handle *devhandle;
	//Convince boogieboard to enter tablet mode
	if(boogieboard){
		int error = libusb_open(boogieboard, &devhandle);
		if(error){
			printf("Error : %d",error);
			goto exit;
		}
		// Dettach kernel drivers
		printf("Attempting to dettach drivers\n");
		if(libusb_kernel_driver_active(devhandle, 1)) error = libusb_detach_kernel_driver(devhandle,1);
		if(error){
			printf("Error : %d\n",error);
			goto exit_close;
		}
		if(libusb_kernel_driver_active(devhandle, 0)) error = libusb_detach_kernel_driver(devhandle,0);
		if(error){
			printf("Error : %d\n",error);
			goto exit_close;
		}
		//Attempt to kick device into tablet mode
		unsigned char control_code[] = {5,0,3};
		while(1){
			error = libusb_control_transfer(devhandle,  0x21, 0x09, 0x0305, 1, control_code, 3, 100);
			if(error > 0) break;
			if(error != LIBUSB_ERROR_TIMEOUT){
				printf("[%d] Something happened.., Can't send control packet\n", error);
				goto exit_close;
			}
			printf("[%d] Timed out retrying...\n",error);
		}
		printf("Board is now in tablet mode\n");
		//Retrieve  interrupt endpoint
		struct libusb_config_descriptor* config;
		error = libusb_get_config_descriptor(boogieboard,0,&config);
		if(error < 0) {
			printf("Failed to retrieve descriptor\n");
		}
		struct libusb_interface_descriptor interrupt = config->interface[1].altsetting[0];
		int i = 0;
		//Initiallize libevdev
		struct libevdev *dev;
		struct libevdev_uinput *uidev;
		struct input_absinfo _x,_y,_p;
		_x.maximum = 5000000;
		_x.minimum = 0;
		_x.fuzz = 0;
		_x.flat = 0;
		_x.resolution = 0;
		_y.maximum = 13442;
		_y.minimum = 0;
		_y.fuzz = 0;
		_y.flat = 0;
		_y.resolution = 0;
		_p.maximum = 1023;
		_p.minimum = 0;
		_p.fuzz = 0;
		_p.flat = 0;
		_p.resolution = 0;
		dev = libevdev_new();
		libevdev_set_name(dev, "BoogieBoard");
		libevdev_enable_event_type(dev, EV_ABS);
		libevdev_enable_event_code(dev, EV_ABS, ABS_X, &_x);
		//libevdev_set_abs_maximum(dev,ABS_X,5000000);
		libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &_y);
		//libevdev_set_abs_maximum(dev,ABS_Y,13442);
		libevdev_enable_event_code(dev, EV_ABS, ABS_PRESSURE, &_p);
		//libevdev_set_abs_maximum(dev,ABS_PRESSURE,1024);
		libevdev_enable_event_type(dev, EV_KEY);
		libevdev_enable_event_code(dev, EV_KEY, BTN_TOUCH, NULL);
		libevdev_enable_event_code(dev, EV_KEY, BTN_STYLUS2, NULL);
		int fd = open("/dev/uinput", O_RDWR);
		error = libevdev_uinput_create_from_device(dev,
                                         fd,
                                         &uidev);
		//libevdev_free(dev);
		if(error) {
			printf("[libevdev:%d]failed to create device", error);
			goto exit_close;
		}
		while(1){
			unsigned char data[8];
			int transferred = 0;
			//Read endpoint	
			error = libusb_interrupt_transfer(devhandle,interrupt.endpoint[0].bEndpointAddress,data,8,&transferred,10);
			if(error != LIBUSB_ERROR_TIMEOUT && error < 0){
				printf("[%d] Stopping now...\n", error);
				break;
			} 
			if(error == 0){
				int x,y,pressure,touch,stylus;
				x = (int) data[0] | (int) data[1] << 8 | (int) data[2] << 16;
				y = (int) data[3] | (int) data[4] << 8;
				pressure = (int) data[5] | (int) data[6] <<8;
				touch = data[7] & 0x01;
				stylus = (data[7] & 0x02)>>1;
				printf("(%d, %d, %d, %d)\n",x,y,pressure,data[7]);
				libevdev_uinput_write_event(uidev, EV_ABS, ABS_X, x);
				libevdev_uinput_write_event(uidev, EV_ABS, ABS_Y, y);
				libevdev_uinput_write_event(uidev, EV_ABS, ABS_PRESSURE, pressure);
				libevdev_uinput_write_event(uidev, EV_KEY, BTN_TOUCH, touch);
				libevdev_uinput_write_event(uidev, EV_KEY, BTN_STYLUS2, stylus);
				libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
			}	
		}
		//Trying to be a responsible citizen and clear up the mess
		libevdev_uinput_destroy(uidev);
		libusb_attach_kernel_driver(devhandle,1);
		libusb_attach_kernel_driver(devhandle,0);
exit_close:
        	libusb_close(devhandle);
	}
	else {
		printf("Boogie board not found... exiting.\n");
	}
exit:
	//Clean up
	libusb_free_device_list(devs,1);
	libusb_exit(NULL);
	return  0;
}
