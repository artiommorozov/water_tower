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

#define RADIO_AUX	IOPORT_CREATE_PIN(PORTB, 1)
#define LEDPIN		IOPORT_CREATE_PIN(PORTB, 0)

#define STATUS_RADIO_REQ_PIN IOPORT_CREATE_PIN(PORTD, 2)
#define STATUS_RADIO_OK_PIN  IOPORT_CREATE_PIN(PORTD, 3)

#define RADIO_MD0	IOPORT_CREATE_PIN(PORTC, 4)
#define RADIO_MD1	IOPORT_CREATE_PIN(PORTC, 5)

#define WATER_LOW	IOPORT_CREATE_PIN(PORTC, 2)
#define WATER_HIGH	IOPORT_CREATE_PIN(PORTC, 3)
#define WATER_LOW_INT	PCINT10
#define WATER_HIGH_INT	PCINT11

#define API_LOW_REQ		"VESNINO WATER LOW\r\n"
#define API_LOW_RESP	"VESNINO LOW ACK\r\n"

#define API_HIGH_REQ	"VESNINO WATER HIGH\r\n"
#define API_HIGH_RESP	"VESNINO HIGH ACK\r\n"


#endif // CONF_BOARD_H
