#ifndef STATS_DISPLAYER_HPP
#define STATS_DISPLAYER_HPP

#include "timer.hpp"

class StatsDisplayer : public RepeatingTimer
{
public:
    StatsDisplayer(): RepeatingTimer(1000) {};
    bool act()
    {
        printf("%u successful registers, %u failed registers\n", success, fail);
        return true;
    }

    uint32_t success;
    uint32_t fail;

};
 
extern StatsDisplayer* stats_displayer;

#endif
