#include "timer.hpp"
#include <re.h>

RepeatingTimer::RepeatingTimer(unsigned int interval_ms) :
    _ms_per_tick(interval_ms),
    _tick(0)
{};

void RepeatingTimer::static_timer_fn(void* arg)
{
    RepeatingTimer* t = static_cast<RepeatingTimer*>(arg);

    t->timer_fn();
}

void RepeatingTimer::start()
{
    _start_time = tmr_jiffies();
    tmr_init(&_timer);
    tmr_start(&_timer, 0, static_timer_fn, this);
}

uint64_t RepeatingTimer::ms_since_start()
{
    return tmr_jiffies() - _start_time;
}

double RepeatingTimer::seconds_since_start()
{
    return (double)ms_since_start() / 1000.0;
}


void RepeatingTimer::timer_fn()
{
    uint64_t next_tick = _start_time + (_ms_per_tick * _tick);
    int diff = next_tick - tmr_jiffies();
    if (diff > 0)
    {
        //printf("A: Rescheduling %p for %d ms\n", this, diff);
        tmr_start(&_timer, diff, static_timer_fn, this);
        return;
    }

    _tick++;
    bool reschedule_timer = act();

    if (reschedule_timer)
    {
        int ms_to_sleep = diff + _ms_per_tick;
        if (ms_to_sleep < 0)
            ms_to_sleep = 0;
        //printf("B: Rescheduling %p for %d ms\n", this, ms_to_sleep);
        tmr_start(&_timer, ms_to_sleep, static_timer_fn, this);
    }
}


