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
 * -# Minimal main function that starts with a call to board_init() * -# "Insert application code here" comment
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
#include <lcd.h>
#include <clock.h>
#include <conf_board.h>
#include <time.h>

#include <radio.h>
#include <radio_commands.h>
#include <ds18b20.h>

static volatile int log_request = 0;
static time_t motor_last_stop = 0, motor_last_start = 0, motor_next_stop = 0;

//#define TESTMODE

#ifndef TESTMODE
static const time_t force_start_if_no_radio = 60UL * 60 * 24; 
static const time_t force_stop_if_no_radio = 60UL * 60 * 3; 
static const time_t force_start_in_winter_each = 60UL * 60 * 3; 
static const time_t winter_runtime = 60 * 10;
static const int winter_max_temp = -5;
#else
static const time_t force_start_if_no_radio = 60;
static const time_t force_stop_if_no_radio = 60;
static const time_t force_start_in_winter_each = 50;
static const time_t winter_runtime = 40;
static const int winter_max_temp = 25;
#endif

enum PumpStatus
{
	StartedByTower = 1,
	StartedByTimeout = 2,
	StartedByColdTemp = 3,
	Stopped = 4,
};
 
static enum PumpStatus pump_status = Stopped;

static unsigned char temp_sensor_rom[0x20];

static void resetRadioWakeInterrupt(void)
{
	if (!ioport_get_pin_level(RADIO_AUX))
		PCMSK1 &= ~(1 << WAKE_BY_RADIO_INT);
}

ISR(PCINT1_vect)
{
	resetRadioWakeInterrupt();
}

ISR(PCINT0_vect)
{
	log_request = 1;
}

/*
static void megaSleep(void)
{
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);

	sleep_enable();
	sleep_bod_disable();
	sei();

	sleep_cpu();

	sleep_disable();
}*/

static void displayMsg(const char *v, const char *action)
{
	push_msg(v, action);
	show_msg(0);
}

static void motorStart(const char *msg, time_t runtime, enum PumpStatus new_status)
{
	if (pump_status != Stopped
		&& !(new_status != StartedByColdTemp
			&& pump_status == StartedByColdTemp))
	{
		displayMsg("Повторный пуск", "пуск");
		return;
	}
	
	ioport_set_pin_high(MOTOR_PIN);
	pump_status = new_status;
	
	time(&motor_last_start);
	motor_next_stop = motor_last_start + runtime;
	
	displayMsg(msg, "пуск");
}

static void motorStop(const char *msg)
{
	ioport_set_pin_low(MOTOR_PIN);
	
	if (pump_status != Stopped)
	{
		pump_status = Stopped;
		time(&motor_last_stop);
	}
	
	displayMsg(msg, "стоп");
}

static int handleRadioSilence(void)
{
	time_t now;
	time(&now);
	
	if (pump_status == Stopped
		&& now - motor_last_stop > force_start_if_no_radio)
	{
		char buf[0x20];
		sprintf(buf, "Нет сигнала %dч", hours(now - motor_last_stop));
		motorStart(buf, force_stop_if_no_radio, StartedByTimeout);
	}
	else if (pump_status != Stopped
		&& now > motor_next_stop)
	{
		char buf[0x20];
		sprintf(buf, "Таймаут %dм", minutes(motor_next_stop - motor_last_start));
		motorStop(buf);
	}
	else
		return 0;
	
	return 1;
}

static int readTemp(void)
{
	return ds18b20_read_temp(temp_sensor_rom);
}

static void periodicColdTimeStart(void)
{
	int temp = readTemp();
	
	time_t now;	
	time(&now);
	
	if (temp < winter_max_temp
		&& pump_status == Stopped
		&& now - motor_last_stop > force_start_in_winter_each)
	{
		char buf[0x20];
		sprintf(buf, "%d *C, перелив", temp);
		motorStart(buf, winter_runtime, StartedByColdTemp);
	}
}

static void checkReqAndReply(void)
{
	const char *requests[] = { API_LOW_REQ, API_HIGH_REQ, NULL };
	int bytes, rtime;
	time_t now;	
	time(&now);
		
	switch (usartIsRecvComplete(requests, &bytes, &rtime))
	{
	case 0: 
	{
		usartReply(API_LOW_RESP); 
		motorStart("Мало воды", force_stop_if_no_radio, StartedByTower);
		
		return;
	} break;
	
	case 1: 
	{ 
		usartReply(API_HIGH_RESP);
		
		if (pump_status == StartedByColdTemp)
			displayMsg("Башня полная", "игнр");
		else 
			motorStop("Башня полная");
			
		return;
	} break;
	}
	
	if (!handleRadioSilence())
		periodicColdTimeStart();
}

static void enableLogButtonInterrupt(void)
{
	cli();
	PCMSK0 |= (1 << LOG_BUTTON_INT);
	PCICR |= 1 << PCIE0;
	sei();
}

#define watchdogReset() asm volatile ("wdr")

static void delay_5s(void)
{
	watchdogReset();
	delay_s(5);
	watchdogReset();
}

static void scanTempSensor(void)
{
	int num_roms = 1;
	unsigned char *roms[] = { temp_sensor_rom };

	ds18b20_scan(roms, &num_roms);
		
	if (num_roms)
	{
		int t;
		char buf[0x20];
		sprintf(buf, "%02X%02X%02X%02X%02X%02X%02X%02X", temp_sensor_rom[0], temp_sensor_rom[1], temp_sensor_rom[2],
			temp_sensor_rom[3], temp_sensor_rom[4], temp_sensor_rom[5],
			temp_sensor_rom[6], temp_sensor_rom[7]);
			
		displayMsg(buf, "TROM");
		delay_5s();
		
		t = readTemp();
		sprintf(buf, "T=%d", t);
		displayMsg(buf, "TEMP");
		delay_5s();
		
#ifdef TESTMODE
		displayMsg("TESTMODE", "ATTN");
		delay_5s();
#endif
	}
	else
		displayMsg("Перезагрузка", "T???");
}

static void enableMCUWatchdog(void)
{
	cli();
	watchdogReset();
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = (1<<WDE) | (1<<WDP3) | (1<<WDP0);	
	sei();
}

int main (void)
{
	int lcd_history_line = 0;
	
	sysclk_init();
	board_init();
	clock_init(BOARD_EXTERNAL_CLK, WATCHDOG_PIN);
	enableMCUWatchdog();
	
	sei();
	lcd_init();

	usartRecvInit(RADIO_AUX, RADIO_MD0, RADIO_MD1);
	enableWakeByRadioInterrupt(WAKE_BY_RADIO_INT);
	enableLogButtonInterrupt();

	ds18b20_init(TEMP_PIN);
	
	time(&motor_last_stop);
	time(&motor_last_start);

	scanTempSensor();	
	
	while (1)
	{
		checkReqAndReply();
		
		if (log_request && !lcd_history_line)
		{	
			log_request = 0;
			lcd_reset();
				
			lcd_history_line = show_msg(++lcd_history_line);
		}
		else if (lcd_history_line)
			lcd_history_line = show_msg(++lcd_history_line);
		else 
			show_msg(0);
			
		delay_5s();
	}
}
