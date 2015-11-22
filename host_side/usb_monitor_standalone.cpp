// g++ usb_monitor.cpp usb_monitor_standalone.cpp -o usb_monitor -lhidapi-libusb -std=gnu++11

#include "usb_monitor.h"

int main(int argc, char**argv){
	usb_monitor_main(argc, argv);
}