#include <avr/interrupt.h>
#include <time.h>
#include <sysclk.h>
#include <clock.h>

static volatile uint16_t v = 0;
static uint16_t interrupts_per_sec = 0;
static int initialized = 0;

ISR (TIMER0_OVF_vect)
{
	if (++v >= interrupts_per_sec)
	{	
		v = 0;
		system_tick();
	}
}

int get_clock(void)
{
	
	return v;
}

void clock_init(uint32_t clk_freq)
{
	if (initialized)
		return;
	
	interrupts_per_sec = clk_freq / 1024 / 256; 
	
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

