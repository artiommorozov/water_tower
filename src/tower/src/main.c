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

#ifdef DEBUG
#define delay_s(x) 
#define delay_ms(x) 
#else
#include <delay.h>
#endif

#include <stdio.h>
#include <string.h>

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <radio.h>
#include <radio_commands.h>
#include <clock.h>

//#define TESTMODE
//#define TESTMODE_NOCOMM

#ifdef TESTMODE
static const time_t check_timeout_sec = 15,
					repeat_crit_level_sec = 30 * 3,
					repeat_crit_level_no_reply_sec = 30;
#else
static const time_t check_timeout_sec = 60 * 2, // 2min between level checks (idle time)
					repeat_crit_level_sec = 60 * 60, // 1hr to repeat low/high level if it keeps and we got ack from pump
					repeat_crit_level_no_reply_sec = 60 * 2 * 3; // 6min to repeat low/high if it keeps and we didn't get ack
#endif

static void procIdle(uint32_t cycles)
{
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	sleep_enable();
	sleep_bod_disable();
	
	while (cycles--)
		sleep_cpu();
	
	sleep_disable();
}

#ifdef TESTMODE_NOCOMM

static int reportLevel(const char *req, const char *resp)
{
	ioport_set_pin_high(STATUS_RADIO_OK_PIN);
	ioport_set_pin_high(STATUS_RADIO_REQ_PIN);
	delay_s(2);
	ioport_set_pin_low(STATUS_RADIO_REQ_PIN);
	ioport_set_pin_low(STATUS_RADIO_OK_PIN);
	return 1;
}

#else

static int reportLevel(const char *req, const char *resp)
{
	const int radio_retry = 2;
	int retry_timeout = 30;
	int ret;
	
	for (int i = 1; !(ret = usartReqWaitAck(req, resp, STATUS_RADIO_REQ_PIN)) && i < radio_retry; ++i)
	{
		delay_s(1);
		ioport_set_pin_low(STATUS_RADIO_REQ_PIN);
		procIdle(clock_sec_to_interrupts(retry_timeout));
	}
	
	if (ret)
		ioport_set_pin_high(STATUS_RADIO_OK_PIN);
	
	delay_s(2);
	ioport_set_pin_low(STATUS_RADIO_REQ_PIN);
	ioport_set_pin_low(STATUS_RADIO_OK_PIN);
	
	return ret;
}

#endif

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
	const int treshold = (cycles * 3) / 4;
	
	for (int i = 0; i < cycles; ++i)
	{
		if (lowLevelTriggered())
			++low;

		if (highLevelTriggered())
			++high;
			
		delay_ms(20);
	}
	
	if (low > treshold 
		&& high > treshold)
	{
		return LevelGood;		
	}
	else if (low > treshold)
		return LevelLow;
	else if (high > treshold)
		return LevelHigh;
	
	return LevelGood;
}

static int timeoutReached(time_t wait_start, time_t expires_at)
{
	time_t now;
	time(&now);
	
	return ((expires_at > wait_start && now >= expires_at)
		|| (expires_at < wait_start && (now >= expires_at && now < wait_start)));	
}

static void sleepUntilLevelChanges(enum WaterLevel prev_level, int last_level_ack)
{		
	time_t enter_time, repeat_time;

	time(&enter_time);
	repeat_time = enter_time + (last_level_ack ? repeat_crit_level_sec : repeat_crit_level_no_reply_sec);

	do 
	{
		procIdle(clock_sec_to_interrupts(check_timeout_sec));
	} while ((checkWaterLevel() == prev_level)
		&& (prev_level == LevelGood
			|| !timeoutReached(enter_time, repeat_time)));
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
		int ack = 0;
		enum WaterLevel level;
		switch (level = checkWaterLevel())
		{
		case LevelLow: { blink(1); ack = reportLowLevel(); } break;
		case LevelGood: { blink(2);  } break;
		case LevelHigh: { blink(3); ack = reportHighLevel(); } break;
		}

		sleepUntilLevelChanges(level, ack);
	}
}
