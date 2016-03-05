// g++ usb_monitor.cpp led_matrix_host.cpp easy_bmp/*.cpp -o matrix_host -lhidapi-libusb -std=gnu++11

#include <cstdio>
#include <cstdint>
#include <hidapi/hidapi.h>
#include "usb_monitor.h"
#include <thread>
#include <cstring>
#include <unistd.h>
#include "easy_bmp/EasyBMP.h"
#include <sys/stat.h>
#include <vector>

static const int8_t row_map[8] = {7,6,5,4,2,3,0,1};
//static const int8_t col_map[8] = {0,1,2,3,7,6,5,4};
//static const int8_t col_map[8] = {0,1,2,3,4,5,6,7};
static const int8_t col_map[8] = {7,6,5,4,0,1,2,3};

#define N_ROWS  16
#define N_COLS 16



#define N_BRIGHT_LEVELS (3)

#define BLUE 0
#define RED 1
#define GREEN 2

#define N_COLORS 3


typedef uint8_t Pixel_Buffer_t[N_ROWS][N_COLS/8][N_BRIGHT_LEVELS][N_COLORS];

#ifndef set_bit
#define set_bit(vec, bit) ((vec) |= (1<<(bit)))
#endif
#ifndef clear_bit
#define clear_bit(vec, bit) ((vec) &= ~(1<<(bit)))
#endif


static void _write_pixel_color(
				Pixel_Buffer_t* buffer, 
				int8_t color, 
				int8_t col, int8_t row, 
				int8_t col_bit, 
				uint8_t brightness){
	for(uint8_t cur_level= 0;cur_level<N_BRIGHT_LEVELS;cur_level++){
		if(brightness > cur_level){
			set_bit((*buffer)[row][col/8][cur_level][color], col_bit);
		}else{
			clear_bit((*buffer)[row][col/8][cur_level][color], col_bit);
		}
	}
}

static void pixel_buffer_write_pixel(
				Pixel_Buffer_t*buffer,
				int8_t col, int8_t row,
				int8_t r, int8_t g, int8_t b){

	// hACK TO FIX orientation
	col = N_COLS - col;
	int8_t tmp = col;
	col = row;
	row = tmp;

	int8_t r_int = row / 8;
	int8_t r_mod = row % 8;
	row = row_map[r_mod] + r_int*8;
	int8_t col_bit = col_map[col%8];
	_write_pixel_color(buffer, RED, col, row, col_bit, r);
	_write_pixel_color(buffer, GREEN, col, row, col_bit, g);
	_write_pixel_color(buffer, BLUE, col, row, col_bit, b);
}

static void pixel_buffer_clear(
				Pixel_Buffer_t*buffer, 
				int8_t r, int8_t g, int8_t b){
	for(int8_t row=0; row<N_ROWS; row++){
		for(int8_t col=0; col<N_COLS; col++){
			pixel_buffer_write_pixel(buffer,col,row,r,g,b);
		}
	}
}


static void pbuff_hor_stripe(
				Pixel_Buffer_t* pbuff,
				int8_t row,
				int8_t r, int8_t g, int8_t b){
	pixel_buffer_clear(pbuff, 0,0,0);
	for(int col=0; col<N_COLS; col++){
		pixel_buffer_write_pixel(pbuff, col, row, r, g, b);
	}
}

static void pbuff_ver_stripe(
				Pixel_Buffer_t* pbuff,
				int8_t col,
				int8_t r, int8_t g, int8_t b){
	pixel_buffer_clear(pbuff, 0,0,0);
	for(int row=0; row<N_ROWS; row++){
		pixel_buffer_write_pixel(pbuff, col, row, r, g, b);
	}
}



#define REPORT_OUT_SIZE     145
#define HALF_PIX_BUFFER			144
// Pixel_Buffer_t pixel_buff;

const uint16_t vendor_id = 0xA134;
const uint16_t product_id = 0x0001;

static int send_pixel_buffer(hid_device* device, Pixel_Buffer_t* pix_buffer){
	int bytes_written;
	uint8_t data_buffer[REPORT_OUT_SIZE+1];
	data_buffer[0] = 0;

	data_buffer[1] = 0;
	memcpy(data_buffer+2, pix_buffer, HALF_PIX_BUFFER);
	bytes_written = hid_write(device, (const unsigned char*)data_buffer,REPORT_OUT_SIZE+1);
	if(bytes_written != REPORT_OUT_SIZE+1){
		return 1;
	}

	data_buffer[1] = 1;
	memcpy(data_buffer+2, ((uint8_t*)pix_buffer)+HALF_PIX_BUFFER, HALF_PIX_BUFFER);
	bytes_written = hid_write(device, (const unsigned char*)data_buffer,REPORT_OUT_SIZE+1);
	if(bytes_written != REPORT_OUT_SIZE+1){
		return 1;
	}
	return 0;
}

void hor_stripe_animation(hid_device* device){
	Pixel_Buffer_t frames[N_ROWS];
	for(int i=0; i<N_ROWS; i++){
		pbuff_hor_stripe(&frames[i], i, 3,3,3);
	}
	for(int f=0; true; f=(f+1)%N_ROWS){
		send_pixel_buffer(device, &frames[f]);
		sleep(1);
	}
}

void ver_stripe_animation(hid_device* device){
	Pixel_Buffer_t frames[N_COLS];
	pbuff_ver_stripe(&frames[0], 0, 3,3,3);
	
	for(int i=1; i<N_COLS; i++){
		pbuff_ver_stripe(&frames[i], i, 3,3,3);
	}
	for(int f=0; true; f=(f+1)%N_COLS){
		send_pixel_buffer(device, &frames[f]);
		sleep(1);
	}
}


// four possible values for each color of each pixel:
// 0, 99, cc, ff


static inline int8_t _255_to_4(uint8_t b){
	if(b<0x99) return 0;
	else if (b<0xcc) return 1;
	else if (b<0xff) return 2;
	return 3;
}


void bmp_to_pbuff(Pixel_Buffer_t* pbuff, BMP bmp){
	if(bmp.TellHeight() != N_ROWS){
		// error
	}
	if(bmp.TellWidth() != N_COLS){
		// error
	}

	for(int y=0; y<N_ROWS; y++){
		for(int x=0; x<N_COLS; x++){
			RGBApixel * pix = bmp(x,y);
			pixel_buffer_write_pixel(
					pbuff, 
					x,y,
					_255_to_4(pix->Red),
					_255_to_4(pix->Green),
					_255_to_4(pix->Blue));
		}
	}
}


// int main(int argc, char* argv[]){
// 	BMP test_bmp;
// 	test_bmp.ReadFromFile("test.bmp");
	
// 	for(int y=0; y<test_bmp.TellHeight(); y++){
// 		for(int x=0; x<test_bmp.TellWidth(); x++){
// 			RGBApixel * pix = test_bmp(x,y);
// 			printf("[%2d,%2d] : (%3hx,%3hx,%3hx)\n", x,y, pix->Red, pix->Green, pix->Blue);
// 		}
// 	}
// }

bool file_exists(const std::string& name){
	struct stat buffer;
	return (stat (name.c_str(), &buffer) == 0); 
}


void loop_animation(hid_device * device, Pixel_Buffer_t * animation, int nframes){
	printf("Looping animation.\n");
	for(int n=0; true; n=(n+1)%nframes){
		int error = send_pixel_buffer(device, &animation[n]);
		if(error != 0){
			fprintf(stderr, "Error sending data. Disconnected.\n");
			return;
		}
	}
}

int main(){
	hid_init();
	hid_device* device = hid_open(vendor_id,product_id,NULL);
	if(device==NULL){
		fprintf(stderr, "Failed to open device.\n");
		exit(1);
	}

	// std::thread monitor(usb_monitor_print_loop, device);


	// std::string paths[] = {"stupid", "test.bmp", "animation", "animation/0.bmp", "animation/"};

	std::string path = "animation";

	char char_buffer[256];

	int frame_count = 0;
	while(true){
		sprintf(char_buffer,"%s/%d.bmp", path.c_str(), frame_count);
		if( ! file_exists((const char*)char_buffer)){
			break;
		}
		frame_count++;
	}
	Pixel_Buffer_t * animation = (Pixel_Buffer_t *) malloc(frame_count * sizeof(Pixel_Buffer_t));

	for(int frame_num=0; frame_num<frame_count; frame_num++){
		BMP bmp;
		sprintf(char_buffer,"%s/%d.bmp", path.c_str(), frame_num);
		printf("Loading frame: %s\n", char_buffer);
		bmp.ReadFromFile(char_buffer);
		bmp_to_pbuff( &animation[frame_num], bmp);
	}


	loop_animation(device, animation, frame_count);

	// for(int i=0; i<5; i++){
	// 	std::cout << paths[i] ;
	// 	std::cout << file_exists(paths[i]) << std::endl;
	// }



//	int * x = new int;
	// Pixel_Buffer_t pix_buffer;
	// pixel_buffer_clear(&pix_buffer,3,1,0);
	// send_pixel_buffer(device, &pix_buffer);
	// hor_stripe_animation(device);

	// Pixel_Buffer_t pix_buffer;
	// BMP test_bmp;
	// test_bmp.ReadFromFile("test.bmp");
	// bmp_to_pbuff(&pix_buffer, test_bmp);
	// send_pixel_buffer(device, &pix_buffer);

	// printf("pixel_buffer_sent\n");
	// monitor.join();
}