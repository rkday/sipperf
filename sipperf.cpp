#include <pthread.h>
#include <string.h>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include <re.h>
#include <vector>

#include "sipua.hpp"

/* called when challenged for credentials */
static int auth_handler(char **user, char **pass, const char *realm, void *arg);
static void register_handler(int err, const struct sip_msg *msg, void *arg);
static std::vector<struct sip*> glb_sip;                 /* SIP stack          */

struct tmr base_timer;

static int count = 0;
static int sip_stacks = 0;

/* called when all sip transactions are completed */
static void exit_handler(void *arg)
{
    (void)arg;

    sip_stacks--;
   if (sip_stacks == 0)
       re_cancel();

    /* stop libre main loop */
}




struct sip* get_sip_stack()
{
    static int idx = 0;
    glb_sip.resize(25);
    idx++;
    idx = idx % 20;
    if (glb_sip[idx] == NULL)
    {
        struct sa laddr;
        sip_alloc(&glb_sip[idx], NULL, 32, 32, 32,
                    "ua demo v" VERSION " (" ARCH "/" OS ")",
                    exit_handler, NULL);
        // fetch local IP address
        net_default_source_addr_get(AF_INET, &laddr);
        // listen on random port 
        sa_set_port(&laddr, 0);
        sip_transp_add(glb_sip[idx], SIP_TRANSP_TCP, &laddr);
        sip_stacks++;
 
    }
    return glb_sip[idx];
}

static std::vector<SIPUE*> ues;

static void cleanup()
{

    for (SIPUE* a : ues)
    {
        delete a;
    }

    for (struct sip* sip : glb_sip)
    {
        if (sip)
            sip_close(sip, false);
    }
}

static void timer_fn(void* arg)
{
    printf("Called on timer!\n");
    count++;
    if (count < 10)
    {
        SIPUE* a = new SIPUE("sip:127.0.0.1",
                             "sip:1234@127.0.0.1",
                             "1234@127.0.0.1",
                             "secret");
        a->register_ue();
        ues.push_back(a);
        tmr_start(&base_timer, 1000, timer_fn, NULL);
    }
    else
    {
        cleanup();
    }
}

int main(int argc, char *argv[])
{
    int err; /* errno return values */

    /* initialize libre state */
    err = libre_init();
    fd_setsize(50000);
    if (err) {
        re_fprintf(stderr, "re init failed: %s\n", strerror(err));
    }

    re_init_timer_heap();
    tmr_init(&base_timer);
    tmr_start(&base_timer, 0, timer_fn, NULL);
    re_main(NULL);
    printf("End of loop\n");
    /* clean up/free all state */
    for (struct sip* sip : glb_sip)
    {
        if (sip)
            mem_deref(sip);
    }
    glb_sip.clear();

    /* free librar state */
    libre_close();

    /* check for memory leaks */
    //tmr_debug();
    mem_debug();

out:
    return err;
}
