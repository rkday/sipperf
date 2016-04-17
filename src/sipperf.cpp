#include <pthread.h>
#include <string.h>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include <re.h>
#include <vector>

#include "timer.hpp"
#include "uamanager.hpp"
#include "stats_displayer.hpp"
#include "sipua.hpp"
#include "stack.hpp"
#include "csv.h"
#include "docopt.h"

static std::vector<SIPUE*> ues;
StatsDisplayer* stats_displayer;
                        
class InitialRegistrar : public RepeatingTimer
{
public:
    InitialRegistrar(uint rps) : RepeatingTimer(10), _registers_per_sec(rps) {};

    bool act();
    bool everything_registered() { return _actual_ues_registered == ues.size(); }
private:
    uint64_t _actual_ues_registered = 0;
    int _registers_per_sec = 700;
};

class CallScheduler : public RepeatingTimer
{
public:
    CallScheduler(InitialRegistrar* registrar) : RepeatingTimer(500), _registrar(registrar) {};

    bool act();
private:
    InitialRegistrar* _registrar;
};

class Cleanup : public RepeatingTimer
{
public:
    Cleanup() : RepeatingTimer(15000) {};

    bool act();
};


static void cleanup()
{
   

    mem_debug();
}


bool Cleanup::act()
{
    static int times = 0;
    printf("Cleanup running\n");
    if (times == 1)
    {
        for (SIPUE* a : ues)
        {
            delete a;
        }
    } else if (times == 2)
    {
        close_sip_stacks(false);
        free_sip_stacks();
    }
    else if (times == 10)
    {
        re_cancel();
    }
    times++;
    return true;
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
    static int times = 0;
    if (_registrar->everything_registered())
    {
        SIPUE* caller = UAManager::get_instance()->get_ua_free_for_call();
        SIPUE* callee = UAManager::get_instance()->get_ua_free_for_call();
        if (caller && callee) {
        printf("%s calls %s\n", caller->uri().c_str(), callee->uri().c_str());
        caller->call(callee->uri());
        times++;
        return (times < 3);
//        return false;
        } else {
            printf("Not enough registered UEs to make call\n");
        }
    }

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
    int rps;
    if (args.rps != NULL)
    {
        rps = std::atoi(args.rps);
    } else
    {
        printf("Defaulting to 10 RPS\n");
        rps = 10;
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
   
    InitialRegistrar registering_timer(rps);
    CallScheduler call_scheduler(&registering_timer);
    stats_displayer = new StatsDisplayer();
    Cleanup c;

    stats_displayer->start();
    call_scheduler.start();
    registering_timer.start();
    c.start();
    

    create_sip_stacks(1);
    re_main(NULL);
    printf("End of loop\n");
    
    //free_sip_stacks();
    libre_close();

    /* check for memory leaks */
    //tmr_debug();
    mem_debug();
    delete stats_displayer;

    return 0;
}
