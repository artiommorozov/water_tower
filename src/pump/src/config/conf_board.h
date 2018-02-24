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

#define RADIO_AUX	IOPORT_CREATE_PIN(PORTC, 2)
#define RADIO_MD0	IOPORT_CREATE_PIN(PORTC, 4)
#define RADIO_MD1	IOPORT_CREATE_PIN(PORTC, 3)

#define LCD_LIGHT	IOPORT_CREATE_PIN(PORTC, 5)
#define LCD_RS		IOPORT_CREATE_PIN(PORTC, 0)
#define LCD_RW		IOPORT_CREATE_PIN(PORTD, 3)
#define LCD_E		IOPORT_CREATE_PIN(PORTC, 1)
#define LCD_DB7		IOPORT_CREATE_PIN(PORTD, 4)
#define LCD_DB6		IOPORT_CREATE_PIN(PORTD, 5)
#define LCD_DB5		IOPORT_CREATE_PIN(PORTD, 6)
#define LCD_DB4		IOPORT_CREATE_PIN(PORTD, 7)
#define WAKE_BY_RADIO_INT PCINT10
#define LOG_BUTTON_INT	  PCINT1
#define LOG_BUTTON_PIN	  IOPORT_CREATE_PIN(PORTB, 1)
#define MOTOR_PIN		  IOPORT_CREATE_PIN(PORTC, 5)

// also adjust asm in owibitfunctions.h and owipolled.h
#define TEMP_PIN		  IOPORT_CREATE_PIN(PORTB, 0)


#define API_LOW_REQ		"VESNINO WATER LOW\r\n"
#define API_LOW_RESP	"VESNINO LOW ACK\r\n"

#define API_HIGH_REQ	"VESNINO WATER HIGH\r\n"
#define API_HIGH_RESP	"VESNINO HIGH ACK\r\n"

#define F_CPU BOARD_EXTERNAL_CLK

#endif // CONF_BOARD_H
