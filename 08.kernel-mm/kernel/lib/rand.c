#include "rand.h"
#include "timer.h"

static uint32_t seed = 0;
static uint32_t rand_a = 33550336;
static uint32_t rand_c = 8128;
static uint32_t rand_m = 0x80000000; // 2 ^ 31

void rand_seed(uint32_t new_seed)
{
    seed = new_seed;
}

void rand_seed_with_time()
{
    rand_seed(getTick());
}

uint32_t rand()
{
    if (seed == 0)
    {
        rand_seed_with_time();
    }

    seed = (seed * rand_a + rand_c) % rand_m;
    return seed;
}

uint32_t rand_range(uint32_t min, uint32_t max)
{
    return (uint32_t)((double)rand() / rand_m * (max - min) + min);
}
