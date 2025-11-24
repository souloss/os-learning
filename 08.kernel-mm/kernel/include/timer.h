#ifndef TIMER_H
#define TIMER_H

#include "types.h"

#define TIMER_FREQUENCY 50

void init_timer(uint32_t frequency);

uint32_t getTick();

#endif
