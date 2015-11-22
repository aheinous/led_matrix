/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */


#include <asf.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static volatile bool hid_enabled = false;


static void usbprintf(const char*format, ...){
	if( ! hid_enabled){
		return;
	}
	char buffer[UDI_HID_REPORT_IN_SIZE];
	va_list args;
	va_start(args, format);
	vsprintf(buffer,format,args);
	udi_hid_generic_send_report_in( (uint8_t *) buffer);
}

	
// _______________________________ L E D   M A T R I X ___________________________
	
static const int8_t row_map[8] = {7,6,5,4,2,3,0,1};
static const int8_t col_map[8] = {0,1,2,3,7,6,5,4};

#define N_ROWS  16
#define N_COLS 16

static const int16_t bright_delay_usecs[] = {1,60,250};

#define N_BRIGHT_LEVELS (sizeof(bright_delay_usecs)/sizeof(bright_delay_usecs[0]))

#define BLUE 0
#define RED 1
#define GREEN 2

#define N_COLORS 3


typedef uint8_t Pixel_Buffer_t[N_ROWS][N_COLS/8][N_BRIGHT_LEVELS][N_COLORS];

static  Pixel_Buffer_t  _buffer_A, _buffer_B;

static volatile Pixel_Buffer_t * cur_pixel_buffer = (volatile Pixel_Buffer_t *) &( _buffer_A);
static volatile Pixel_Buffer_t * other_pixel_buffer =  (volatile Pixel_Buffer_t *)& _buffer_B;



static void led_matrix_init_pins(void){
	ioport_set_pin_dir(ROW_CLOCK_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(ROW_DATA_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(COL_CLOCK_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(COL_DATA_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(SPI_SS_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(STROBE_PIN, IOPORT_DIR_OUTPUT);
}

static void led_matrix_init_module(void){
	struct spi_device spi_conf;
	spi_master_init(&LED_MATRIX_SPI);
	spi_master_setup_device(&LED_MATRIX_SPI, &spi_conf, SPI_MODE_0, LED_MATRIX_BAUD, 0);
	spi_enable(&LED_MATRIX_SPI);
}

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
	for(int8_t cur_level= 0;cur_level<N_BRIGHT_LEVELS;cur_level++){
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

static void led_matrix_cycle_row_clock(void){
	ioport_set_pin_level(ROW_CLOCK_PIN, IOPORT_PIN_LEVEL_HIGH);
	delay_us(2);
	ioport_set_pin_level(ROW_CLOCK_PIN, IOPORT_PIN_LEVEL_LOW);
}

static void led_matrix_cycle_strobe(void){
	ioport_set_pin_level(STROBE_PIN, IOPORT_PIN_LEVEL_HIGH);
	delay_us(2);
	ioport_set_pin_level(STROBE_PIN, IOPORT_PIN_LEVEL_LOW);
}

static void led_matrix_transfer_row(int8_t row_counter){
	uint8_t * data;
	for(int8_t bright_level=0; bright_level<N_BRIGHT_LEVELS; bright_level++){
		for(int8_t col_counter=N_COLS/8-1; col_counter>=0; col_counter--){
			data = (uint8_t *)(*cur_pixel_buffer)[row_counter][col_counter][bright_level];		
			spi_write_packet(
					&LED_MATRIX_SPI,
					data,
					N_COLORS);							
		}
		led_matrix_cycle_strobe();
		// brightness delay here
		delay_us(bright_delay_usecs[bright_level]);
	}
}

static void led_matrix_transfer_img(void){
	ioport_set_pin_level(ROW_DATA_PIN, IOPORT_PIN_LEVEL_LOW);
	//delay_us(20);
	led_matrix_cycle_row_clock();
	ioport_set_pin_level(ROW_DATA_PIN, IOPORT_PIN_LEVEL_HIGH);
	
	int8_t row_counter = 0;
	led_matrix_transfer_row(row_counter);
	for(row_counter=1; row_counter<N_ROWS; row_counter++){
		led_matrix_cycle_row_clock();
		led_matrix_transfer_row(row_counter);
	}
}


// ___________________________ E N D   L E D   M A T R I X _____________________________

// _________________________________ U S B   H I D _____________________________________

#define LED0 IOPORT_CREATE_PIN(PORTA,4)



bool hid_cb_enable(){
	
	hid_enabled = true;
	return true;
}

void hid_cb_disable(){
	hid_enabled = false;
}

// 145
// 288

void hid_cb_report_out(uint8_t* report){
	size_t half_buffer = sizeof(Pixel_Buffer_t)/2;
	if(report[0] == 0){
		memcpy(other_pixel_buffer, report+1, half_buffer);
	}else if(report[0] == 1){
		memcpy(((uint8_t*)other_pixel_buffer)+half_buffer, report+1, half_buffer);
		Pixel_Buffer_t*tmp = (Pixel_Buffer_t*) cur_pixel_buffer;
		cur_pixel_buffer = other_pixel_buffer;
		other_pixel_buffer = (volatile Pixel_Buffer_t*)tmp;
	}
	
		
	
	
	//bool good = true;
	//for(int i=0; i<UDI_HID_REPORT_OUT_SIZE; i++){
		//if(report[i] != 0xAB) good = false;
	//}
	//if(good){
		//usbprintf("report was good");
	//}else{
		//usbprintf("report was bad");
	//}
	
}

static void usb_init(void){
	udc_start();
}



int main (void){

	sysclk_init();
	irq_initialize_vectors();
	cpu_irq_enable();
	usb_init();
	board_init();

	ioport_set_pin_dir(LED0, IOPORT_DIR_OUTPUT);
	//ioport_set_pin_level(LED0, IOPORT_PIN_LEVEL_HIGH);
	
	led_matrix_init_pins();
	led_matrix_init_module();

	pixel_buffer_clear((Pixel_Buffer_t*) cur_pixel_buffer, 0,0,0);

	//char data[UDI_HID_REPORT_IN_SIZE];
	//const char * str = "Hello, how are you?";
	
	//sprintf(data, "buffer size: %d", sizeof(Pixel_Buffer_t));
	//strcpy(&data[0], str);
	while(true){
		//if(hid_enabled){
			led_matrix_transfer_img();
			//delay_s(1);
			//udi_hid_generic_send_report_in( (uint8_t *) data);
			//usbprintf("hello\n");
		//}
		

	}
}
