/**
 * \file
 *
 * \brief User board definition template
 *
 */

 /* This file is intended to contain definitions and configuration details for
 * features and devices that are available on the board, e.g., frequency and
 * startup time for an external crystal, external memory devices, LED and USART
 * pins.
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#ifndef USER_BOARD_H
#define USER_BOARD_H

#include <conf_board.h>

// External oscillator settings.
// Uncomment and set correct values if external oscillator is used.

// External oscillator frequency
//#define BOARD_XOSC_HZ          8000000

// External oscillator type.
//!< External clock signal
//#define BOARD_XOSC_TYPE        XOSC_TYPE_EXTERNAL
//!< 32.768 kHz resonator on TOSC
//#define BOARD_XOSC_TYPE        XOSC_TYPE_32KHZ
//!< 0.4 to 16 MHz resonator on XTALS
//#define BOARD_XOSC_TYPE        XOSC_TYPE_XTAL

// External oscillator startup time
//#define BOARD_XOSC_STARTUP_US  500000


#define STROBE_PIN IOPORT_CREATE_PIN(PORTD,0)
#define ROW_CLOCK_PIN IOPORT_CREATE_PIN(PORTD,1)
#define ROW_DATA_PIN IOPORT_CREATE_PIN(PORTD,2)
#define COL_CLOCK_PIN IOPORT_CREATE_PIN(PORTC,7)
#define COL_DATA_PIN IOPORT_CREATE_PIN(PORTC,5)
#define SPI_SS_PIN IOPORT_CREATE_PIN(PORTC,4)
//#define SPI_DUMMY_PIN IOPORT_CREATE_PIN(PORTC,3)
#define LED_MATRIX_SPI SPIC
#define LED_MATRIX_BAUD (1L*1000L*1000L)

#endif // USER_BOARD_H
