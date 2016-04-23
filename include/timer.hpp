#ifndef SIPPERF_TIMER_HPP
#define SIPPERF_TIMER_HPP

#include <stdint.h>
#include <unistd.h>
#include <re.h>

/// Wrapper around a libre timer, to run a function once every N milliseconds.
/// Designed for subclassing - any operation which should happen regularly
/// (registering new UEs, making calls, etc.) should be implemented by
/// subclassing this class and overriding the act() method.
class RepeatingTimer
{
public:
    /// @param interval_ms Number of milliseconds that should pass between each
    /// invocation of this timer's act() function. 
    RepeatingTimer(unsigned int interval_ms);
    virtual ~RepeatingTimer() = default;

    void start();

protected:
    /// Function called regularly on a timer. Designed to be overriden by
    /// subclasses.
    /// 
    /// @return true if this timer should continue to be scheduled, false if it
    /// should stop.
    virtual bool act() = 0;

    /// @return Number of milliseconds since this timer was started.
    uint64_t ms_since_start();

    /// @return Number of seconds since this timer was started.
    double seconds_since_start();

private:
    void timer_fn();

    /// Static function to serve as the timer callback function.
    /// 
    /// @param arg The RepeatingTimer object to call timer_fn on.
    static void static_timer_fn(void* arg);

    unsigned int _ms_per_tick;
    /// Underlying libre timer
    struct tmr _timer;
    uint64_t _start_time;

    /// How many intervals have passed?
    uint64_t _tick;
};

#endif
