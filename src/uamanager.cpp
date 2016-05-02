#include "uamanager.hpp"
#include "useragent.hpp"

UAManager UAManager::_instance;

void UAManager::mark_ua_registered(UserAgent* u)
{
    free_ues[u->name()] = u;
    all_ues[u->name()] = u;
}

void UAManager::mark_ua_not_in_call(UserAgent* u)
{
    if (u != nullptr)
    {
    free_ues[u->name()] = u;
    }
}

void UAManager::mark_ua_in_call(UserAgent* u)
{
    free_ues.erase(u->name());
}


UserAgent* UAManager::get_ua_by_name(std::string name) {
    return all_ues[name];
}

UserAgent* UAManager::get_ua_free_for_call() {
    if (free_ues.empty()) {
        return NULL;
    }

    UserAgent* ret = free_ues.begin()->second;
    mark_ua_in_call(ret);
    return ret;
}

void UAManager::clear()
{
    free_ues.clear();
    all_ues.clear();
}
