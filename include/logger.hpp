#include <iostream>
#include <iomanip>

class IndividualLog
{
public:
    IndividualLog(std::ostream& o, std::string level):
        stream(o)
    {
        char timestamp[64];
        struct timespec tp;
        clock_gettime(CLOCK_REALTIME_COARSE, &tp);
        time_t ts = tp.tv_sec;
        uint16_t milliseconds = tp.tv_nsec / 1000000;
        strftime(timestamp, 64, "%H:%M:%S.", localtime(&ts));
        stream << "["
            << timestamp
            << std::setfill('0') << std::setw(3) << milliseconds
            << "] ";
        if (!level.empty())
        {
            stream << "[" << level << "] ";
        }
    }

    template <typename T> IndividualLog& operator<< (const T& data)
    {
        stream << data;
        return *this;
    }

    ~IndividualLog()
    {
        stream << std::endl;
    }
protected:
    std::ostream& stream;
};


class Logger
{
public:
    IndividualLog warning()
    {
        return IndividualLog(std::cout, "Warning");
    }

    IndividualLog debug()
    {
        return IndividualLog(std::cout, "Debug");
    }
};

int level = 3;
static Logger l;

#define WARNING_LOG(x) if (level > 2) { l.warning() << x; };
#define DEBUG_LOG(x) if (level > 5) { l.debug() << x; };
