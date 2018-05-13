#include <avr/interrupt.h>
#include <ioport.h>
#include <time.h>
#include <sysclk.h>
#include <clock.h>

static volatile uint16_t v = 0;
static uint16_t interrupts_per_sec = 0, strobe_low = 0;
static int initialized = 0;
static uint8_t strobe_pin = 0;

ISR (TIMER0_OVF_vect)
{
	if (++v >= interrupts_per_sec)
	{	
		v = 0;
		system_tick();
	}
	else if (v == strobe_low)
	{
		if (strobe_pin)
		{
			ioport_set_pin_low(strobe_pin);
			ioport_set_pin_high(strobe_pin);	
		}
	}		
}

int get_clock(void)
{
	
	return v;
}

uint32_t clock_sec_to_interrupts(uint32_t sec)
{
	return sec * interrupts_per_sec;
}

void clock_init(uint32_t clk_freq, uint8_t a_stobe_pin)
{
	if (initialized)
		return;
	
	strobe_pin = a_stobe_pin;
	ioport_set_pin_high(strobe_pin);	
	
	interrupts_per_sec = clk_freq / 1024 / 256; // for 1.84Mhz about 7 ints per sec
	strobe_low = interrupts_per_sec / 2;
	
	set_system_time(0);
	
	sysclk_enable_peripheral_clock(&TCCR0A);
	
	GTCCR = (1 << TSM) | (1 << PSRSYNC);
	
	TCCR0A = 0;
	TCCR0B = (1 << CS02) | (1 << CS00); // prescaler 1024
	TIMSK0 |= (1 << TOIE0); // then extra division by 256 with this overflow
	
	GTCCR = 0;
	
	initialized = 1;
}

int hours(time_t v)
{
	return v / 3600; 
}

int minutes(time_t v) 
{
	return v / 60;
}

