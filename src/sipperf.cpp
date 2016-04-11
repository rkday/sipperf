#include <pthread.h>
#include <string.h>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include <re.h>
#include <vector>

#include "sipua.hpp"
#include "stack.hpp"

static uint64_t max_ues = 10000;
static std::vector<SIPUE*> ues;

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

    virtual bool act() = 0;

protected:
    unsigned int _ms_per_tick;
    struct tmr _timer;
    uint64_t _start_time;
    uint64_t _tick;
};

class InitialRegistrar : public RepeatingTimer
{
public:
    InitialRegistrar() : RepeatingTimer(10) {};

    bool act();
    bool everything_registered() { return _actual_ues_registered == max_ues; }
private:
    uint64_t _actual_ues_registered = 0;
    double _registers_per_ms = 0.7;
};

class CallScheduler : public RepeatingTimer
{
public:
    CallScheduler(InitialRegistrar* registrar) : RepeatingTimer(1000), _registrar(registrar) {};

    bool act();
private:
    InitialRegistrar* _registrar;
};


static void cleanup()
{
    for (SIPUE* a : ues)
    {
        delete a;
    }
    
    close_sip_stacks(false);
}

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

bool InitialRegistrar::act()
{
    if (_actual_ues_registered == max_ues)
    {
 //       cleanup();
        return false;
    }

    uint64_t expected_ues_registered = ms_since_start() * _registers_per_ms;
    expected_ues_registered = std::min(expected_ues_registered, max_ues);
    int ues_to_register = expected_ues_registered - _actual_ues_registered;
    //printf("%d UEs to register\n", ues_to_register);
    for (int ii = 0; ii < ues_to_register; ii++)
    {
       SIPUE* a = new SIPUE("sip:127.0.0.1",
                             "sip:1234@127.0.0.1",
                             "1234@127.0.0.1",
                             "secret");
        a->register_ue();
        ues.push_back(a);
        _actual_ues_registered++;
    }

    return true;
}

bool CallScheduler::act()
{
    if (_registrar->everything_registered())
        printf("A calls B\n");
    else
        printf("Still waiting...\n");

    return true;

}
int main(int argc, char *argv[])
{
    int err; /* errno return values */

    /* initialize libre state */
    err = libre_init();
    fd_setsize(50000);
    if (err) {
        re_fprintf(stderr, "re init failed: %s\n", strerror(err));
    }

    re_init_timer_heap();
   
    InitialRegistrar registering_timer;
    CallScheduler call_scheduler(&registering_timer);

    call_scheduler.start();
    registering_timer.start();
    

    create_sip_stacks(20);
    re_main(NULL);
    printf("End of loop\n");
    
    free_sip_stacks();
    libre_close();

    /* check for memory leaks */
    //tmr_debug();
    mem_debug();

    return 0;
}
