#ifndef SIPPERF_TIMER_HPP
#define SIPPERF_TIMER_HPP

#include <stdint.h>
#include <unistd.h>
#include <re.h>

class RepeatingTimer
{
public:
    RepeatingTimer(unsigned int ms_per_tick) :
        _ms_per_tick(ms_per_tick),
        _tick(0)
    {};
    virtual ~RepeatingTimer() = default;

    void start();
    static void static_timer_fn(void* arg);
    void timer_fn();

    uint64_t ms_since_start();
    double seconds_since_start();

    virtual bool act() = 0;

protected:
    unsigned int _ms_per_tick;
    struct tmr _timer;
    uint64_t _start_time;
    uint64_t _tick;
};


#endif
