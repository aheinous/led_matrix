// g++ usb_monitor.cpp usb_monitor_standalone.cpp -o usb_monitor -lhidapi-libusb -std=gnu++11

#include "usb_monitor.h"
#include <cstdio>
#include <cstdint>
#include <tclap/CmdLine.h>
#include <unistd.h>


static const int STR_BUFFER_SIZE = 255;

void usb_monitor_list_hid_devices(){
	struct hid_device_info *devs;
	struct hid_device_info *cur_dev;

	devs = hid_enumerate(0,0);

	for(cur_dev=devs; cur_dev!=NULL; cur_dev=cur_dev->next){
		printf("%04hx:%04hx %ls %ls\n",
				cur_dev->vendor_id,
				cur_dev->product_id,
				cur_dev->manufacturer_string,
				cur_dev->product_string);
	}
	hid_free_enumeration(devs);
}


void usb_monitor_print_loop(hid_device* device){
	unsigned char str_buffer[STR_BUFFER_SIZE+1];

	while(true){
		int bytes_read = hid_read(device, str_buffer, STR_BUFFER_SIZE);
		if(bytes_read == -1){
			fprintf(stderr, "Error during read. Disconnected.\n");
			return;
		}
		str_buffer[bytes_read] = 0; // make sure string is null terminated
		printf("%s",(const char *)str_buffer);	
	}
}

void usb_monitor_loop(uint16_t vendor_id, uint16_t product_id, bool live_forever){
	hid_device* device = NULL;
	bool first_attempt = true;
	char connect_str[40];
	sprintf(connect_str,"Opening device %04hx:%04hx ... ", vendor_id, product_id);
	const char * left_fmt = "%-30s";
	const char * right_fmt = "%20s\n";
	do{
		if(first_attempt){
			printf(left_fmt, connect_str);
			fflush(stdout);
		} 
		device = hid_open(vendor_id, product_id, NULL);
		if(device != NULL){
			printf(right_fmt, " ... Device opened.");
			usb_monitor_print_loop(device);
			first_attempt = true;
		}else{
			if(first_attempt) {printf(right_fmt," ... Failed." );}
			if(live_forever){
				if(first_attempt) printf(left_fmt, "Will keep trying forever ... ");
				fflush(stdout);
				sleep(1);
			} 	
			first_attempt = false;	
		}
	}while(live_forever);
}

void usb_monitor_main(int argc, char** argv){

	try{
		TCLAP::CmdLine cmd("USB monitor for debugging code on XMegas or whatever.", ' ', "0.1");
		
		TCLAP::ValueArg<std::string> vendor_id_arg(
				"v",
				"vendor",
				"Vendor ID of target USB device.", 
				false,
				"0",
				"Four digit hex number",
				cmd
			);

		TCLAP::ValueArg<std::string> product_id_arg(
				"p",
				"product",
				"Product ID of target USB device.", 
				false,
				"0",
				"Four digit hex number",
				cmd
			);

		TCLAP::SwitchArg list_arg(
				"l",
				"list",
				"List available HID devices",
				cmd
			);

		TCLAP::SwitchArg forever_arg(
				"f",
				"forever",
				"Live forever or until killed. Try to reconnect after loosing connection",
				cmd
			);
		cmd.parse(argc, argv);

		if(list_arg.getValue()){
			usb_monitor_list_hid_devices();
		}

		bool live_forever = forever_arg.getValue();

		uint16_t vendor_id = std::stoi(vendor_id_arg.getValue(), nullptr, 16);
		uint16_t product_id = std::stoi(product_id_arg.getValue(), nullptr, 16);

		if(vendor_id != 0 && product_id != 0){
			usb_monitor_loop(vendor_id, product_id, live_forever);
		}


	}catch (TCLAP::ArgException &e)  // catch any exceptions
	{ std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; }

}


