#ifndef OWN_CLOCK_H_INCLUDED
#define OWN_CLOCK_H_INCLUDED

#include <time.h>

void clock_init(uint32_t clk_freq, uint8_t a_stobe_pin);
int get_clock(void);

int hours(time_t v);
int minutes(time_t v);

#endif
