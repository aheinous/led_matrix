// g++ usb_monitor.cpp usb_monitor_standalone.cpp -o usb_monitor -lhidapi-libusb -std=gnu++11

#ifndef _USB_MONITOR_H
#define _USB_MONITOR_H
#include <cstdint>
#include <hidapi/hidapi.h>

void usb_monitor_main(int argc, char** argv);
void usb_monitor_loop(uint16_t vendor_id, uint16_t product_id, bool live_forever);
void usb_monitor_print_loop(hid_device* device);
void usb_monitor_list_hid_devices();

#endif