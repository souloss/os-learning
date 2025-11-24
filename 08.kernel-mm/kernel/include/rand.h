#ifndef RAND_H
#define RAND_H

#include "types.h"

void rand_seed(uint32_t seed);

void rand_seed_with_time();

uint32_t rand();

uint32_t rand_range(uint32_t min, uint32_t max);

#endif
