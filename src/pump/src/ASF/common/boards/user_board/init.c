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

static void init_watchdog(void)
{
	ioport_set_pin_high(WATCHDOG_PIN);
	ioport_set_pin_low(WATCHDOG_PIN);
	ioport_set_pin_high(WATCHDOG_PIN);
}

void board_init(void)
{
	const int out_pins[] = { RADIO_MD0, RADIO_MD1, MOTOR_PIN, WATCHDOG_PIN };
	const int in_pins[] = { RADIO_AUX, LOG_BUTTON_PIN };
		
	ioport_init();

	for (int i = 0; i < sizeof(out_pins) / sizeof(out_pins[0]); ++i)
	{
		ioport_set_pin_dir(out_pins[i], IOPORT_DIR_OUTPUT);
		ioport_set_pin_low(out_pins[i]);
	}
	
	for (int i = 0; i < sizeof(in_pins) / sizeof(in_pins[0]); ++i)
	{
		ioport_set_pin_dir(in_pins[i], IOPORT_DIR_INPUT);
		ioport_set_pin_mode(in_pins[i], IOPORT_MODE_PULLUP);
	}
	
	init_watchdog();
}
