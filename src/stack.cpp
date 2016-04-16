#include "stack.hpp"

#include <vector>
#include <stdint.h>
#include <re.h>

static std::vector<struct sip*> sip_stacks;
static int active_sip_stacks = 0;

static void exit_handler(void *arg);

void create_sip_stacks(int how_many)
{
	struct sa nsv[16];
	struct dnsc *dnsc = NULL;
	uint32_t nsc = ARRAY_SIZE(nsv);
	dns_srv_get(NULL, 0, nsv, &nsc);
	dnsc_alloc(&dnsc, NULL, nsv, nsc);
    for (int i = 0; i < how_many; i++)
    {
        struct sip* sip;
        struct sa laddr;
        sip_alloc(&sip, dnsc, 32, 32, 32,
                    "ua demo v" VERSION " (" ARCH "/" OS ")",
                    exit_handler, NULL);

        // Listen on random port on local IP
        net_default_source_addr_get(AF_INET, &laddr);
        sa_set_port(&laddr, 0);
        sip_transp_add(sip, SIP_TRANSP_TCP, &laddr);

        sip_stacks.push_back(sip);
        active_sip_stacks++;
    }
}

void close_sip_stacks(bool force)
{
    for (struct sip* sip : sip_stacks)
    {
        sip_close(sip, force);
    }

}

void free_sip_stacks()
{
    for (struct sip* sip : sip_stacks)
    {
        mem_deref(sip);
    }

    sip_stacks.clear();
}

struct sip* get_sip_stack()
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


