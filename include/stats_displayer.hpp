#ifndef STATS_DISPLAYER_HPP
#define STATS_DISPLAYER_HPP

#include "timer.hpp"

class StatsDisplayer : public RepeatingTimer
{
public:
    StatsDisplayer(): RepeatingTimer(1000) {};
    bool act()
    {
        printf("%u successful registers\n", success_reg);
        printf("%u failed registers\n", fail_reg);
        printf("%u calls successfully set up\n", success_call);
        printf("%u calls failed\n", failed_call);
        return true;
    }

    uint32_t success_reg = 0;
    uint32_t success_call = 0;
    uint32_t fail_reg = 0;
    uint32_t failed_call = 0;

};
 
extern StatsDisplayer* stats_displayer;

#endif
