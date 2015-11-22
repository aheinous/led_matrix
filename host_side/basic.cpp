// g++ basic.cpp -o basic -lhidapi-libusb -std=gnu++11

#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include "hidapi/hidapi.h"

const uint16_t vendor_id = 0xA134;
const uint16_t product_id = 0x0001;
const int wstr_buffer_size = 256;
const int str_buffer_size = 512;

hid_device* device = NULL;

void quit(int error_code){
	if (device != NULL){
		printf("Releasing device.\n");
		hid_close(device);
	}
	printf("Exiting HID.\n");
	hid_exit();
	if(error_code==0){
		printf("No errors. Good bye.\n");
	}else{
		fprintf(stderr, "ERROR CODE: %d\n", error_code);
	}
	exit(error_code);
}

int main(void){

	printf("Opening device ...");
	device = hid_open(vendor_id, product_id, NULL);
	if(device==NULL){
		fprintf(stderr, "\nFailed to open device.\n" );
		quit(1);
	}
	printf("... Device opened.\n\n");

	wchar_t wstr_buffer[wstr_buffer_size];
	int error_code;

	error_code = hid_get_manufacturer_string(device, wstr_buffer, wstr_buffer_size);
	if(error_code!=0){
		fprintf(stderr, "Failed to read manufacturer_string.\n" );
		quit(1);
	}
	printf("%-15s %35S\n", "Manufacturer:", wstr_buffer);

	error_code = hid_get_product_string(device, wstr_buffer, wstr_buffer_size);
	if(error_code!=0){
		fprintf(stderr, "Failed to read product_string.\n" );
		quit(1);
	}
	printf("%-15s %35S\n","Product:", wstr_buffer);

	error_code = hid_get_serial_number_string(device, wstr_buffer, wstr_buffer_size);
	if(error_code!=0){
		fprintf(stderr, "Failed to read serial_number_string.\n" );
		quit(1);
	}
	printf("%-15s %35S\n","Serial Number:", wstr_buffer);

	printf("\n");


	unsigned char to_write[146] = {0};
	for(int i=1; i<146; i++){
		to_write[i] = 0xAB;
	}
	hid_write(device, (const unsigned char *) to_write, 146);


	hid_set_nonblocking(device, false);
	unsigned char str_buffer[str_buffer_size+1];

	printf("Entering read loop.\n");
	while(true){
		int bytes_read = hid_read(device, str_buffer, str_buffer_size);
		if(bytes_read == -1){
			fprintf(stderr, "Error during read.\n");
			quit(1);
		}
		str_buffer[bytes_read] = 0; // make sure
		printf("Device:[%2d] \"%s\"\n", bytes_read, str_buffer);

	}




	quit(0);
}


// int main(int argc, char*argv[]){
// 	struct hid_device_info *devs;
// 	struct hid_device_info *cur_dev;

// 	int res;
// 	unsigned char buf[65];
// 	const int MAX_STR = 255;
// 	wchar_t wstr[MAX_STR];

// 	// devs = hid_enumerate(0,0);

	
// 	// for(cur_dev=devs; cur_dev!=NULL; cur_dev=cur_dev->next){
// 	// 	printf("%04hx:%04hx %ls %ls\n",
// 	// 			cur_dev->vendor_id,
// 	// 			cur_dev->product_id,
// 	// 			cur_dev->manufacturer_string,
// 	// 			cur_dev->product_string);
// 	// }
// 	// hid_free_enumeration(devs);
	
// 	handle = hid_open(0xabcd, 0xbeef, NULL);
// 	if(handle==NULL){
// 		fprintf(stderr, "handle is NULL\n");
// 		quit(1);
// 	}
// 	printf("Opened device.\n");
// 	// Read the Manufacturer String
// 	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
// 	printf("Manufacturer String: %ls\n", wstr);

// 	// Read the Product String
// 	res = hid_get_product_string(handle, wstr, MAX_STR);
// 	printf("Product String: %ls\n", wstr);
// 	// printf("b\n");

// 	// Read the Serial Number String
// 	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
// 	printf("Serial Number String: %ls", wstr);
// 	printf("\n");	


// 	// printf("Reading Feature Report.\n");
// 	// buf[0] = 0;
// 	// int bytes_read = hid_get_feature_report(handle, buf, 65);
// 	// if(bytes_read == -1){
// 	// 	fprintf(stderr, "ERROR: Could not read feature report.\n");
// 	// 	quit(1);
// 	// }else{
// 	// 	// printf("F")
// 	// 	for(int i=1; i<bytes_read; i++){
// 	// 		if(i!=1){
// 	// 			printf(", ");
// 	// 		}
// 	// 		printf("%02hx", buf[i]);
// 	// 	}
// 	// 	printf("\n");
// 	// }
// 	// Read requested state

// 	hid_set_nonblocking(handle, 0);

// 	buf[0] = 0;
// 	buf[1] = 5;
// 	res = hid_write(handle, buf, 5);
// 	printf("res: %d\n", res);


// 	int bytes_read = hid_read_timeout(handle, buf, 65, 5000);
// 	if (bytes_read == -1){
// 		fprintf(stderr,"Error: Unable to read.\n");
// 		quit(1);
// 	}else if(bytes_read == 0){
// 		printf("timeout.");
// 	}else{
// 		// Print out the returned buffer.
// 		for (int i=0; i < bytes_read; i++){
// 			printf("buf[%d]: %02hx\n", i, buf[i]);
// 		}
		
// 	}



// 	quit(0);
// }