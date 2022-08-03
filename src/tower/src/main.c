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
#include <avr/power.h>
#include <radio.h>
#include <radio_commands.h>
#include <clock.h>

//#define TESTMODE
//#define TESTMODE_NOCOMM

#ifdef TESTMODE
static const time_t check_timeout_sec = 10,
					repeat_crit_level_sec = 30 * 3,
					repeat_crit_level_no_reply_sec = 10;
#else
static const time_t check_timeout_sec = 60 * 2, // 2min between level checks (idle time)
					repeat_crit_level_sec = 60 * 60, // 1hr to repeat low/high level if it keeps and we got ack from pump
					repeat_crit_level_no_reply_sec = 60 * 10; // 10min to repeat low/high if it keeps and we didn't get ack
#endif

static int error = 0;

static int ledPins[] = { LEDPIN1, LEDPIN2, LEDPIN3, LEDPIN4 };

static void delay_manySec(int sec)
{
	for (int i = 0; i < sec; ++i)
	{
		delay_s(1);
	}
}

#undef delay_s
#define delay_s delay_manySec

static void powerDown(uint32_t cycles)
{	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	sleep_enable();
	sleep_bod_disable();
	
	while (cycles--)
		sleep_cpu();
	
	sleep_disable();
}


static void ledOff(void)
{
	for (int i = 0; i < sizeof(ledPins) / sizeof(ledPins[0]); ++i)
		ioport_set_pin_low(ledPins[i]);
}

static void ledInit(void)
{
	for (int i = 0; i < sizeof(ledPins) / sizeof(ledPins[0]); ++i)
	{
		ioport_set_pin_high(ledPins[i]);
		delay_ms(500);
	}
	
	ledOff();
}

static void showWorkingStatus(void)
{
	if (error)
		ioport_set_pin_high(STATUS_ERR_LED);
	
	ioport_set_pin_high(STATUS_OK_LED);
	delay_ms(100);
	ioport_set_pin_low(STATUS_OK_LED);
	ioport_set_pin_low(STATUS_ERR_LED);
}

#ifdef TESTMODE_NOCOMM

static int reportLevel(const char *req, const char *resp)
{
	delay_s(1);
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
	int i = 0;
		
	while (i++ < radio_retry)
	{
		radioOn(RADIO_POWER);
		ret = usartReqWaitAck(req, resp, STATUS_RADIO_REQ_PIN);
		radioOff(RADIO_POWER);

		if (ret)
			break;
		
		delay_s(1);
		ioport_set_pin_low(STATUS_RADIO_REQ_PIN);
		powerDown(clock_sec_to_interrupts(retry_timeout));
	}
	
	if (ret)
		ioport_set_pin_high(STATUS_RADIO_OK_PIN);

	error = !ret;	
		
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
		showWorkingStatus();
		powerDown(clock_sec_to_interrupts(check_timeout_sec));
	} while ((checkWaterLevel() == prev_level)
		&& (prev_level == LevelGood
			|| !timeoutReached(enter_time, repeat_time)));
}

static int level_change_led_time_s = 3;

static int reportLowLevel(void)
{
	ioport_set_pin_high(STATUS_LED_LOW);
	delay_s(level_change_led_time_s);
	ledOff();
	
	return reportLevel(API_LOW_REQ, API_LOW_RESP);
}

static int reportHighLevel(void)
{
	ioport_set_pin_high(STATUS_LED_LOW);
	ioport_set_pin_high(STATUS_LED_MED);
	ioport_set_pin_high(STATUS_LED_HIGH);
	delay_s(level_change_led_time_s);
	ledOff();
	
	return reportLevel(API_HIGH_REQ, API_HIGH_RESP);
}

static void reportMedLevel(void)
{
	ioport_set_pin_high(STATUS_LED_LOW);
	ioport_set_pin_high(STATUS_LED_MED);
	delay_s(level_change_led_time_s);
	ledOff();
}

static void showNibble(unsigned char t)
{
	if (t & 1)
		ioport_set_pin_high(STATUS_LED_HIGH);
		
	if (t & 2)
		ioport_set_pin_high(STATUS_LED_MED);
		
	if (t & 4)
		ioport_set_pin_high(STATUS_LED_LOW);
						
	if (t & 8)
		ioport_set_pin_high(STATUS_ERR_LED);

	delay_s(1);
	ledOff();	
}

int main (void)
{	
	sysclk_init();
	board_init();

	radioOn(RADIO_POWER);
	usartTransInit(RADIO_AUX, RADIO_MD0, RADIO_MD1);
	radioOff(RADIO_POWER);

	ledInit();	
	clock_init(BOARD_EXTERNAL_CLK, 1);		
	
	power_spi_disable();
	power_timer0_disable();
	power_timer1_disable();
	power_timer2_disable();
	power_twi_disable();	
	sei();
	
#if 0
	// power consumption test
	while (1)
	{
		ioport_set_pin_high(STATUS_OK_LED);
		radioOn(RADIO_POWER);
		delay_s(5);
		ioport_set_pin_low(STATUS_OK_LED);
		radioOff(RADIO_POWER);
		powerDown(60);
	}
#endif
	
#if 0
	// params test
	while (1)
	{
			unsigned char cfgReq[4] = { 0xc1, 0xc1, 0xc1, 0 };
			int respLen = 0;
			char resp[7];
			
			respLen = usartReqCfg((char*) cfgReq, resp, sizeof(resp), STATUS_RADIO_REQ_PIN);
			
			if (respLen <= 0)
			{
				ioport_set_pin_high(STATUS_ERR_LED);				
			}
			else
			{
				for (unsigned char *p = (unsigned char*) resp; respLen--; ++p)
				{
					showNibble(*p & 0xf);
					showNibble(*p >> 4);
					
					delay_ms(200);
					ioport_set_pin_high(STATUS_ERR_LED);
					delay_ms(200);
					ioport_set_pin_low(STATUS_ERR_LED);
					delay_ms(200);
				}	
			}
			
			delay_s(5);
	}
#endif	
	
#if 0
	// quick test

	for (; 1; showWorkingStatus())
	{
		powerDown(clock_sec_to_interrupts(10));
		
		reportHighLevel();
		delay_s(5);
		reportMedLevel();
		delay_s(5);
		reportLowLevel();

		ledOff();
		//powerDown(check_timeout_sec);
		powerDown(50);
	}
#endif

	while (1)
	{
		int ack = 0;
		enum WaterLevel level;
		switch (level = checkWaterLevel())
		{
		case LevelLow: { ack = reportLowLevel(); } break;
		case LevelGood: { reportMedLevel(); } break;
		case LevelHigh: { ack = reportHighLevel(); } break;
		}

		sleepUntilLevelChanges(level, ack);
	}
}
