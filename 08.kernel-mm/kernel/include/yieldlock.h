#ifndef YIELDLOCK_H
#define YIELDLOCK_H

#include "types.h"

#define LOCKED_YES 1
#define LOCKED_NO 0

// yieldlock gives up CPU when it cannot get lock.
// Since a thread 'yield' action is involved, it apparently can NOT be used in interrupt context.
typedef struct yieldlock
{
    volatile uint32_t lock;
} yieldlock_t;

// ****************************************************************************
void yieldlock_init(yieldlock_t *splock);
void yieldlock_lock(yieldlock_t *splock);
bool_t yieldlock_trylock(yieldlock_t *splock);
void yieldlock_unlock(yieldlock_t *splock);

#endif
