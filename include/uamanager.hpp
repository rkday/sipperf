#ifndef UAMANAGER_HPP
#define UAMANAGER_HPP

#include <unordered_map>

class UserAgent;

/// Singleton class with a complete view of all UEs, including:
//
// - what ones are registered
// - what ones are currently in calls
// - what ones are free to make/receive calls
// - what Contact addresses map to what user agent instances (for handling incoming calls)
class UAManager {
public:
    static UAManager* get_instance() { return &_instance;}
    void mark_ua_registered(UserAgent* u);
    void mark_ua_unregistered(UserAgent* u);

    UserAgent* get_ua_free_for_call();
    UserAgent* get_ua_by_name(std::string name);

    void mark_ua_in_call(UserAgent* u);
    void mark_ua_not_in_call(UserAgent* u);

    void clear();
private:
    UAManager() = default;
    std::unordered_map<std::string, UserAgent*> free_ues;
    std::unordered_map<std::string, UserAgent*> all_ues;
    static UAManager _instance;
};

#endif
