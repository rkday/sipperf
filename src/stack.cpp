#include "stack.hpp"
#include "sipua.hpp"
#include "uamanager.hpp"

#include <vector>
#include <stdint.h>
#include <re.h>
#include <utility>

static std::vector<std::pair<struct sip*, struct sipsess_sock*>> sip_stacks;
static int active_sip_stacks = 0;
static struct dnsc *dnsc = NULL;

static void exit_handler(void *arg);

static void static_connect_handler(const struct sip_msg *msg, void *arg) {
    std::string name(msg->uri.user.p, msg->uri.user.l);
    SIPUE* ue = UAManager::get_instance()->get_ua_by_name(name);
    ue->connect_handler(msg);
}


void create_sip_stacks(int how_many)
{
	struct sa nsv;
    sa_set_str(&nsv, "127.0.0.1", 53);
	dnsc_alloc(&dnsc, NULL, &nsv, 1);
    for (int i = 0; i < how_many; i++)
    {
        struct sip* sip;
        struct sipsess_sock *sess_sock;
        struct sa laddr;
        sip_alloc(&sip, dnsc, 32, 32, 32,
                    "ua demo v" VERSION " (" ARCH "/" OS ")",
                    exit_handler, NULL);

        // Listen on random port on local IP
        net_default_source_addr_get(AF_INET, &laddr);
        sa_set_port(&laddr, 0);
        sip_transp_add(sip, SIP_TRANSP_TCP, &laddr);
        sipsess_listen(&sess_sock, sip, 32, static_connect_handler, NULL);

        sip_stacks.push_back(std::make_pair(sip, sess_sock));
        active_sip_stacks++;
    }
}

void close_sip_stacks(bool force)
{
    for (auto sip : sip_stacks)
    {
        sip_close(sip.first, force);
    }
}

void free_sip_stacks()
{
    for (auto sip : sip_stacks)
    {
        mem_deref(sip.first);
        mem_deref(sip.second);
    }

    sip_stacks.clear();
    mem_deref(dnsc);
}

std::pair<struct sip*, struct sipsess_sock*> get_sip_stack()
{
    static int counter = 0;
    counter++;
    int idx = counter % sip_stacks.size();
    return sip_stacks[idx];
}

static void exit_handler(void *arg)
{
    // All SIP transactions on this stack have finished
    active_sip_stacks--;
    if (active_sip_stacks == 0)
    {
        // Stop the main loop when all SIP transactions end
        re_cancel();
    }
}


