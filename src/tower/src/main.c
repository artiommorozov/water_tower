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
#include <radio_commands.h>
#include <clock.h>

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

static void blink(int count)
{
	for (int k = 0; k < count; ++k)
	{
		ioport_set_pin_high(LEDPIN);
		delay_s(1);
		ioport_set_pin_low(LEDPIN);
		delay_s(1);
	}
}

static bool lowLevelTriggered(void)
{
	return !ioport_get_pin_level(WATER_LOW);
}

static bool highLevelTriggered(void)
{
	return !ioport_get_pin_level(WATER_HIGH);
}

static enum WaterLevel checkWaterLevel(void)
{
	int low = 0, high = 0;
	const int cycles = 100;
	
	for (int i = 0; i < cycles; ++i)
	{
		if (lowLevelTriggered())
			++low;

		if (highLevelTriggered())
			++high;
			
		delay_ms(20);
	}
	
	if (low > cycles / 2)
		return LevelLow;
	else if (high > cycles / 2)
		return LevelHigh;
	
	return LevelGood;
}

static void megaSleep(void)
{
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	sleep_enable();
	sleep_bod_disable();
	sei();
	
	sleep_cpu();
	
	sleep_disable();
}

static int timeoutReached(time_t wait_start, time_t expires_at)
{
	time_t now;
	time(&now);
	
	return ((expires_at > wait_start && now >= expires_at)
		|| (expires_at < wait_start && (now >= expires_at && now < wait_start)));	
}

static void sleepUntilLevelChanges(enum WaterLevel prev_level)
{
	const time_t check_timeout_sec = 30,
		repeat_low_level_sec = 60 * 30;
		
	time_t wait_start, next_check_at, enter_time;
	
	for (time(&enter_time); 
		checkWaterLevel() == prev_level
		&& !(prev_level == LevelLow
			&& timeoutReached(enter_time, enter_time + repeat_low_level_sec));
		)
	{
		time(&wait_start);
		next_check_at = wait_start + check_timeout_sec;

		while (!timeoutReached(wait_start, next_check_at))
			megaSleep();				
	}	
}

static int reportLevel(const char *req, const char *resp)
{
	const int radio_retry = 5;
	int retry_time_base = 30;
	int ret;
		
	for (int i = 0; i < radio_retry && !(ret = usartReqWaitAck(req, resp, STATUS_RADIO_REQ_PIN)); ++i)
	{
		for (int j = 0; j < retry_time_base * (j + 1); ++j)
		{
			delay_s(1);
			ioport_set_pin_low(STATUS_RADIO_REQ_PIN);
		}
	}
		
	if (ret)
		ioport_set_pin_high(STATUS_RADIO_OK_PIN);	
		
	delay_s(2);
	ioport_set_pin_low(STATUS_RADIO_REQ_PIN);
	ioport_set_pin_low(STATUS_RADIO_OK_PIN);	
	
	return ret;
}

static int reportLowLevel(void)
{
	return reportLevel(API_LOW_REQ, API_LOW_RESP);
}

static int reportHighLevel(void)
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
	clock_init(BOARD_EXTERNAL_CLK, 0);
		
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
		case LevelLow: { blink(1); reportLowLevel(); } break;
		case LevelGood: { blink(2);  } break;
		case LevelHigh: { blink(3); reportHighLevel(); } break;
		}

		sleepUntilLevelChanges(level);
	}
}
