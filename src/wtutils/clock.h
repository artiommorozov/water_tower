#ifndef OWN_CLOCK_H_INCLUDED
#define OWN_CLOCK_H_INCLUDED

#include <time.h>

void clock_init(uint32_t clk_freq, int watchdogClock);
int get_clock(void);
uint32_t clock_sec_to_interrupts(uint32_t sec);

int hours(time_t v);
int minutes(time_t v);

#endif
