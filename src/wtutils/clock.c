#include <avr/interrupt.h>
#include <ioport.h>
#include <time.h>
#include <sysclk.h>
#include <clock.h>

static volatile uint16_t v = 0;
static int initialized = 0;
static uint16_t interrupts_per_sec = 1;

ISR (TIMER0_OVF_vect)
{
	if (++v >= interrupts_per_sec)
	{
		v = 0;
		system_tick();
	}
}

ISR (WDT_vect)
{
	system_tick();
}

int get_clock(void)
{
	return v;
}

uint32_t clock_sec_to_interrupts(uint32_t sec)
{
	return sec * interrupts_per_sec;
}

void clock_init(uint32_t clk_freq, int watchdogClock)
{
	if (initialized)
		return;

	set_system_time(0);

	if (watchdogClock)
	{
		cli();
		WDTCSR |= (1 << WDCE) | (1 << WDE);
		WDTCSR = (0 << WDE) | (1 << WDIE) | (1 << WDP2) | (1 << WDP1);
		sei();
		sysclk_enable_peripheral_clock(&TCCR0A);
	}
	else
	{
		sysclk_enable_peripheral_clock(&TCCR0A);
		interrupts_per_sec = clk_freq / 1024 / 256; // for 1.84Mhz about 7 ints per sec
		
		//cli();
		GTCCR = (1 << TSM) | (1 << PSRSYNC);
		TCCR0A = 0;
		TCCR0B = (1 << CS02) | (1 << CS00); // prescaler 1024
		TIMSK0 |= (1 << TOIE0); // then extra division by 256 with this overflow
		GTCCR = 0;
		//sei();
	}	
	
		
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

