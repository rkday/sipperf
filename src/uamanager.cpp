#include "uamanager.hpp"
#include "sipua.hpp"

UAManager UAManager::_instance;

void UAManager::mark_ua_registered(SIPUE* u)
{
    free_ues[u->name()] = u;
    all_ues[u->name()] = u;
}

void UAManager::mark_ua_not_in_call(SIPUE* u)
{
    if (u != nullptr)
    {
    free_ues[u->name()] = u;
    }
}

void UAManager::mark_ua_in_call(SIPUE* u)
{
    free_ues.erase(u->name());
}


SIPUE* UAManager::get_ua_by_name(std::string name) {
    return all_ues[name];
}

SIPUE* UAManager::get_ua_free_for_call() {
    if (free_ues.empty()) {
        return NULL;
    }

    SIPUE* ret = free_ues.begin()->second;
    mark_ua_in_call(ret);
    return ret;
}


