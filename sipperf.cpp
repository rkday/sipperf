
#include <pthread.h>
#include <string.h>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include <re.h>
#include <vector>

static struct dnsc *dnsc = NULL;

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



/* called upon incoming calls */
static void connect_handler(const struct sip_msg *msg, void *arg)
{
}

static struct sip* get_sip_stack()
{
    static int idx = 0;
    glb_sip.resize(25);
    idx++;
    idx = idx % 20;
    if (glb_sip[idx] == NULL)
    {
        struct sa laddr;
        sip_alloc(&glb_sip[idx], dnsc, 32, 32, 32,
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

class SIPUE
{
public:
    SIPUE(std::string registrar,
          std::string uri,
          std::string username,
          std::string password):
        _registrar(registrar),
        _uri(uri),
        _username(username),
        _password(password)
    {}

    ~SIPUE()
    {
        mem_deref(sess_sock);
        mem_deref(reg);
    }

    void register_ue()
    {
        //re_thread_enter();
        my_sip = get_sip_stack();

        sipsess_listen(&sess_sock, my_sip, 32, connect_handler, this);
        sipreg_register(&reg,
                        my_sip,
                        _registrar.c_str(),
                        _uri.c_str(),
                        _uri.c_str(),
                        300,
                        "RKD",
                        NULL,
                        0,
                        0,
                        ::auth_handler,
                        this,
                        false,
                        ::register_handler,
                        this,
                        NULL,
                        NULL);
        //re_thread_leave();
    }
 
/* called when register responses are received */
void register_handler(int err, const struct sip_msg *msg)
{
    if (err)
        re_printf("register error: %s\n", strerror(err));
    //else
        //re_printf("register reply: %u %r\n", msg->scode, &msg->reason);
}

 /* called when challenged for credentials */
 int auth_handler(char **user, char **pass, const char *realm)
{
    int err = 0;
    (void)realm;

    str_dup(user, _username.c_str());
    str_dup(pass, _password.c_str());

    return err;
}

  
private:
    struct sip *my_sip;            /* SIP session        */
    struct sipsess *sess;            /* SIP session        */
    struct sipsess_sock *sess_sock;  /* SIP session socket */
    struct sipreg *reg;              /* SIP registration   */
    struct sa laddr;

    std::string _registrar;
    std::string _uri;
    std::string _username;
    std::string _password;
};

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


static int auth_handler(char **user, char **pass, const char *realm, void *arg)
{
    return ((SIPUE*)arg)->auth_handler(user, pass, realm);
}

/* called when register responses are received */
static void register_handler(int err, const struct sip_msg *msg, void *arg)
{
    ((SIPUE*)arg)->register_handler(err, msg);
}

void* lib_re_thread_func(void* arg)
{
    re_main(NULL);
    printf("End of thread func\n");
}

int main(int argc, char *argv[])
{
    struct sa nsv[16];
    uint32_t nsc;
    int err; /* errno return values */

    /* enable coredumps to aid debugging */
    (void)sys_coredump_set(true);

    /* initialize libre state */
    err = libre_init();
    fd_setsize(50000);
    if (err) {
        re_fprintf(stderr, "re init failed: %s\n", strerror(err));
    }

    nsc = ARRAY_SIZE(nsv);

    /* fetch list of DNS server IP addresses */
    err = dns_srv_get(NULL, 0, nsv, &nsc);
    if (err) {
        re_fprintf(stderr, "unable to get dns servers: %s\n",
                   strerror(err));
    }

    /* create DNS client */
    err = dnsc_alloc(&dnsc, NULL, nsv, nsc);
    if (err) {
        re_fprintf(stderr, "unable to create dns client: %s\n",
                   strerror(err));
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
    mem_deref(dnsc);

    /* free librar state */
    libre_close();

    /* check for memory leaks */
    //tmr_debug();
    //mem_debug();

out:
    return err;
}
