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
#include <delay.h>
#include <stdio.h>
#include <string.h>

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <radio.h>

enum InterruptOnChange
{
	PinTriggered,
	PinReset
};

enum WaterLevel
{
	LevelLow,
	LevelHigh,
	LevelGood	
};

void resetWaterLevelInterrupt(void);
bool lowLevelTriggered(void);
bool highLevelTriggered(void);
enum WaterLevel checkWaterLevel(void);
void megaSleep(void);
int sleepUntilLevelChanges(enum WaterLevel prev_level);
void interruptOnWaterLevel(enum InterruptOnChange water_low, enum InterruptOnChange water_high);
int reportLevel(const char *req, const char *resp);
int reportHighLevel(void);
int reportLowLevel(void);
void blink(int count);

void blink(int count)
{
	for (int k = 0; k < count; ++k)
	{
		ioport_set_pin_high(LEDPIN);
		delay_s(1);
		ioport_set_pin_low(LEDPIN);
		delay_s(1);
	}
}


void resetWaterLevelInterrupt(void)
{
	PCICR &= 0;
}

ISR(PCINT1_vect)
{
	resetWaterLevelInterrupt();
}

void interruptOnWaterLevel(enum InterruptOnChange water_low, enum InterruptOnChange water_high)
{
	PCMSK1 |= (1 << WATER_HIGH_INT) + (1 << WATER_LOW_INT);
	PCICR |= 1 << PCIE1;	
}

bool lowLevelTriggered(void)
{
	return !ioport_get_pin_level(WATER_LOW);
}

bool highLevelTriggered(void)
{
	return !ioport_get_pin_level(WATER_HIGH);
}

enum WaterLevel checkWaterLevel(void)
{
	int low = 0, high = 0;
	const int cycles = 100;
	
	for (int i = 0; i < cycles; ++i)
	{
		if (lowLevelTriggered())
			++low;

		if (highLevelTriggered())
			++high;
			
		delay_ms(10);
	}
	
	if (low > cycles / 2)
	{
		blink(3);
		return LevelLow;
	}
	else if (high > cycles / 2)
	{
		blink(2);
		return LevelHigh;
	}
	
	return LevelGood;
}

void megaSleep(void)
{
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	sleep_enable();
	sleep_bod_disable();
	sei();
	
	sleep_cpu();
	
	sleep_disable();
}

int sleepUntilLevelChanges(enum WaterLevel prev_level)
{
	enum WaterLevel new_level = prev_level;
	
	while (1)
	{
		cli();
		
		switch (prev_level)
		{
		case LevelLow: interruptOnWaterLevel(PinReset, PinTriggered); break;
		case LevelHigh: interruptOnWaterLevel(PinTriggered, PinReset); break;
		case LevelGood: interruptOnWaterLevel(PinTriggered, PinTriggered); break;
		}
		
		if ((new_level = checkWaterLevel()) != prev_level)
		{
			resetWaterLevelInterrupt();
			break;
		}
		
		megaSleep();

		if ((new_level = checkWaterLevel()) == prev_level)
			continue;
			
		cli();
		prev_level = new_level;
		break; 
	} 

	sei();
	return new_level;
}

int reportLevel(const char *req, const char *resp)
{
	const int radio_retry = 5;
	const int retry_timeout_sec = 30;
	int ret;
		
	for (int i = 0; i < radio_retry && !(ret = usartReqWaitAck(req, resp, STATUS_RADIO_REQ_PIN)); ++i)
		for (int j = 0; j < retry_timeout_sec; ++j)
		{
			delay_s(1);
		}
		
	if (ret)
		ioport_set_pin_high(STATUS_RADIO_OK_PIN);	
		
	delay_s(2);
	ioport_set_pin_low(STATUS_RADIO_REQ_PIN);
	ioport_set_pin_low(STATUS_RADIO_OK_PIN);	
	
	return ret;
}

int reportLowLevel(void)
{
	return reportLevel(API_LOW_REQ, API_LOW_RESP);
}

int reportHighLevel(void)
{
	return reportLevel(API_HIGH_REQ, API_HIGH_RESP);
}

static void led_init(void)
{
	ioport_set_pin_high(LEDPIN);
	delay_s(1);
	ioport_set_pin_high(STATUS_RADIO_REQ_PIN);
	delay_s(1);
	ioport_set_pin_high(STATUS_RADIO_OK_PIN);
	delay_s(1);
	ioport_set_pin_low(LEDPIN);
	ioport_set_pin_low(STATUS_RADIO_REQ_PIN);
	ioport_set_pin_low(STATUS_RADIO_OK_PIN);
}

int main (void)
{
	sysclk_init();
	board_init();

	led_init();	
		
	sei();

#if 0	
	// quick test

	while (1)
	{
		reportHighLevel();
		delay_s(5);
		reportLowLevel();
		for (int i = 0; i < 60; ++i) delay_s(1);
	}
#endif

	while (1)
	{
		enum WaterLevel level;
		switch (level = checkWaterLevel())
		{
		case LevelLow: { reportLowLevel(); } break;
		case LevelHigh: { reportHighLevel(); } break;
		case LevelGood: break;
		}

		sleepUntilLevelChanges(level);
	}
}
