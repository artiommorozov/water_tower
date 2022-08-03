/**
 * \file
 *
 * \brief User board configuration template
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#ifndef CONF_BOARD_H
#define CONF_BOARD_H

#define LEDPIN1					IOPORT_CREATE_PIN(PORTD, 2)
#define LEDPIN2					IOPORT_CREATE_PIN(PORTD, 3)
#define LEDPIN3					IOPORT_CREATE_PIN(PORTD, 4)
#define LEDPIN4					IOPORT_CREATE_PIN(PORTD, 5)

#define STATUS_OK_LED LEDPIN1
#define STATUS_ERR_LED LEDPIN4

#define STATUS_LED_LOW LEDPIN3
#define STATUS_LED_MED LEDPIN2
#define STATUS_LED_HIGH LEDPIN1

#define STATUS_RADIO_REQ_PIN	LEDPIN2
#define STATUS_RADIO_OK_PIN		LEDPIN1

#define RADIO_POWER	IOPORT_CREATE_PIN(PORTB, 1)
#define RADIO_AUX	IOPORT_CREATE_PIN(PORTC, 1)
#define RADIO_MD0	IOPORT_CREATE_PIN(PORTC, 2)
#define RADIO_MD1	IOPORT_CREATE_PIN(PORTC, 3)

#define WATER_LOW	IOPORT_CREATE_PIN(PORTD, 6)
#define WATER_HIGH	IOPORT_CREATE_PIN(PORTD, 7)

#endif // CONF_BOARD_H
