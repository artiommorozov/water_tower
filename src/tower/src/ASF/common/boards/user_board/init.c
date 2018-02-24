/**
 * \file
 *
 * \brief User board initialization template
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include <asf.h>
#include <board.h>
#include <conf_board.h>
#include <delay.h>
#include <usart_mega.h>
#include <sysclk.h>
#include <radio.h>

void radioSleep(void);
void blink(int);

void board_init(void)
{
	const int out_pins[] = { RADIO_MD0, RADIO_MD1, LEDPIN, STATUS_RADIO_OK_PIN, STATUS_RADIO_REQ_PIN };
	
	ioport_init();
	ioport_set_pin_dir(WATER_LOW, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(WATER_LOW, IOPORT_MODE_PULLUP);
	ioport_set_pin_dir(WATER_HIGH, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(WATER_HIGH, IOPORT_MODE_PULLUP);
	
	ioport_set_pin_dir(RADIO_AUX, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(RADIO_AUX, IOPORT_MODE_PULLUP);

	for (int i = 0; i < sizeof(out_pins) / sizeof(out_pins[0]); ++i)
	{
		ioport_set_pin_dir(out_pins[i], IOPORT_DIR_OUTPUT);
		ioport_set_pin_low(out_pins[i]);
	}
	
	usartTransInit(RADIO_AUX, RADIO_MD0, RADIO_MD1);
}
