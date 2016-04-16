#include <pthread.h>
#include <string.h>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include <re.h>
#include <vector>

#include "sipua.hpp"
#include "stack.hpp"
#include "csv.h"
#include "docopt.h"

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
    double seconds_since_start();

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
    bool everything_registered() { return _actual_ues_registered == ues.size(); }
private:
    uint64_t _actual_ues_registered = 0;
    double _registers_per_sec = 700;
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

bool InitialRegistrar::act()
{
    if (_actual_ues_registered == ues.size())
    {
 //       cleanup();
        return false;
    }

    uint64_t expected_ues_registered = seconds_since_start() * _registers_per_sec;
    expected_ues_registered = std::min(expected_ues_registered, ues.size());
    int ues_to_register = expected_ues_registered - _actual_ues_registered;
    //printf("%d UEs to register\n", ues_to_register);
    for (int ii = 0; ii < ues_to_register; ii++)
    {
        SIPUE* a = ues[_actual_ues_registered];
        a->register_ue();
        _actual_ues_registered++;
    }

    return true;
}

bool CallScheduler::act()
{
    if (_registrar->everything_registered())
    {
        SIPUE* caller = ues[0];
        SIPUE* callee = ues[1];
        printf("%s calls %s\n", caller->uri().c_str(), callee->uri().c_str());
        caller->call(callee->uri());
    }
    else
        printf("Still waiting...\n");

    return true;

}
int main(int argc, char *argv[])
{
    DocoptArgs args = docopt(argc, argv, /* help */ 1, /* version */ "0.01");
    if (args.target == NULL)
    {
        printf("--target option is mandatory\n");
        printf("%s\n", help_message);
        exit(1);
    }
    printf("%s\n", args.target);


    int err; /* errno return values */

    io::CSVReader<3> in("users.csv");

    std::string sip_uri, username, password;

    while (in.read_row(sip_uri, username, password))
    {
       SIPUE* a = new SIPUE(args.target,
                             sip_uri,
                             username,
                             password);
        ues.push_back(a);
    }

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
