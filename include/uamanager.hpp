#ifndef UAMANAGER_HPP
#define UAMANAGER_HPP

#include <unordered_map>

class SIPUE;

class UAManager {
public:
    static UAManager* get_instance() { return &_instance;}
    void mark_ua_registered(SIPUE* u);
    void mark_ua_unregistered(SIPUE* u);

    SIPUE* get_ua_free_for_call();
    SIPUE* get_ua_by_name(std::string name);

    void mark_ua_in_call(SIPUE* u);
    void mark_ua_not_in_call(SIPUE* u);
private:
    UAManager() = default;
    std::unordered_map<std::string, SIPUE*> free_ues;
    std::unordered_map<std::string, SIPUE*> all_ues;
    static UAManager _instance;
};

#endif
