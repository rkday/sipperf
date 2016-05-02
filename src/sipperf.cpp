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
#include "useragent.hpp"
#include "stack.hpp"
#include "logger.hpp"
#include "csv.h"
#include "docopt.h"

static std::vector<UserAgent*> ues;
StatsDisplayer* stats_displayer;

class CallScheduler;
class Cleanup;

static CallScheduler* call_scheduler;
static Cleanup* cleanup_task;
                        
class InitialRegistrar : public RepeatingTimer
{
public:
    InitialRegistrar(uint rps) :
        RepeatingTimer(10), 
        _actual_ues_registered(0),
        _registers_per_sec(rps) {};

    bool act();
    bool everything_registered() { return _actual_ues_registered == ues.size(); }
private:
    uint64_t _actual_ues_registered;
    int _registers_per_sec;
};

class CallScheduler : public RepeatingTimer
{
public:
    CallScheduler(int cps, uint64_t max_calls) :
        RepeatingTimer(10),
        _actual_calls(0),
        _calls_per_sec(cps),
        _max_calls(max_calls) {};

    bool act();
private:
    uint64_t _actual_calls;
    int _calls_per_sec;
    uint64_t _max_calls;
};

class Cleanup : public RepeatingTimer
{
public:
    Cleanup() : RepeatingTimer(1000) {};

    bool act();
};

bool Cleanup::act()
{
    static int times = 0;
    printf("Cleanup running\n");
    if (times == 1)
    {
        for (UserAgent* a : ues)
        {
            a->unregister();
        }
    } else if (times == 2)
    {
        close_sip_stacks(false);
        free_sip_stacks();
    }
    else if (times == 30)
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
        call_scheduler->start();
        return false;
    }

    uint64_t expected_ues_registered = seconds_since_start() * _registers_per_sec;
    expected_ues_registered = std::min(expected_ues_registered, ues.size());
    int ues_to_register = expected_ues_registered - _actual_ues_registered;

    for (int ii = 0; ii < ues_to_register; ii++)
    {
        UserAgent* a = ues[_actual_ues_registered];
        a->register_ue();
        _actual_ues_registered++;
    }

    return true;
}

bool CallScheduler::act()
{
        uint64_t expected_calls = seconds_since_start() * _calls_per_sec;
        int calls_to_make = expected_calls - _actual_calls;
        
        for (int ii = 0; ii < calls_to_make; ii++)
        {
            UserAgent* caller = UAManager::get_instance()->get_ua_free_for_call();
            UserAgent* callee = UAManager::get_instance()->get_ua_free_for_call();
            if (caller && callee) {
                caller->call(callee->uri());
                _actual_calls++;
            } else {
                printf("Not enough registered UEs to make call\n");
                UAManager::get_instance()->mark_ua_not_in_call(caller);
                UAManager::get_instance()->mark_ua_not_in_call(callee);
                break;
            }
        }

    if (_actual_calls < _max_calls)
    {
        return true;
    }
    else
    {
        cleanup_task->start();
        return false;
    }
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
    int rps = std::atoi(args.rps);
    int cps = std::atoi(args.cps);
    int max_calls = std::atoi(args.max_calls);

    int err; /* errno return values */

    l.set_cflog_file("./cflog.log");
    io::CSVReader<3> in(args.users_file);

    std::string sip_uri, username, password;

    while (in.read_row(sip_uri, username, password))
    {
       UserAgent* a = new UserAgent(args.target,
                             sip_uri,
                             username,
                             password);
        ues.push_back(a);
    }

    printf("Registering %u user agents against registrar %s\n", ues.size(), args.target);

    /* initialize libre state */
    err = libre_init();
    fd_setsize(50000);
    if (err) {
        re_fprintf(stderr, "re init failed: %s\n", strerror(err));
    }

    InitialRegistrar registering_timer(rps);
    call_scheduler = new CallScheduler(cps, max_calls);
    stats_displayer = new StatsDisplayer();
    cleanup_task = new Cleanup();

    stats_displayer->start();
    registering_timer.start();
    

    create_sip_stacks(75);
    re_main(NULL);
    printf("End of loop\n");
    
    libre_close();

    UAManager::get_instance()->clear();
    for (UserAgent* a : ues)
    {
        delete a;
    }

    /* check for memory leaks */
    tmr_debug();
    mem_debug();
    delete stats_displayer;

    return 0;
}
