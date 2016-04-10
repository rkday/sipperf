#include "sipua.hpp"
#include "stack.hpp"

/* called upon incoming calls */
static void connect_handler(const struct sip_msg *msg, void *arg)
{
}

SIPUE::~SIPUE()
{
    mem_deref(sess_sock);
    mem_deref(reg);
}

void SIPUE::register_ue()
{
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
                    static_auth_handler,
                    this,
                    false,
                    static_register_handler,
                    this,
                    NULL,
                    NULL);
}

/* called when register responses are received */
void SIPUE::register_handler(int err, const struct sip_msg *msg)
{
//    if (err)
//        re_printf("register error: %s\n", strerror(err));
    //else
    //re_printf("register reply: %u %r\n", msg->scode, &msg->reason);
}

/* called when challenged for credentials */
int SIPUE::auth_handler(char **user, char **pass, const char *realm)
{
    int err = 0;
    (void)realm;

    str_dup(user, _username.c_str());
    str_dup(pass, _password.c_str());

    return err;
}

int SIPUE::static_auth_handler(char **user, char **pass, const char *realm, void *arg)
{
    return ((SIPUE*)arg)->auth_handler(user, pass, realm);
}

/* called when register responses are received */
void SIPUE::static_register_handler(int err, const struct sip_msg *msg, void *arg)
{
    ((SIPUE*)arg)->register_handler(err, msg);
}


