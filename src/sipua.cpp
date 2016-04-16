#include "sipua.hpp"
#include "stack.hpp"

/* called upon incoming calls */
void SIPUE::static_connect_handler(const struct sip_msg *msg, void *arg)
{
    SIPUE* ue = static_cast<SIPUE*>(arg);
    ue->connect_handler(msg);
}

void SIPUE::connect_handler(const struct sip_msg *msg)
{
	re_printf("connection attempt to %s\n", _uri.c_str());
    (void)sip_treply(NULL, my_sip, msg, 486, "Busy Here");
    return;
}

/* called when SIP progress (like 180 Ringing) responses are received */
static void progress_handler(const struct sip_msg *msg, void *arg)
{
	(void)arg;

	re_printf("session progress: %u %r\n", msg->scode, &msg->reason);
}

/* called when the session fails to connect or is terminated from peer */
static void close_handler(int err, const struct sip_msg *msg, void *arg)
{
	(void)arg;

	if (err)
		re_printf("session closed: %d\n", err);
	else
		re_printf("session closed: %u %r\n", msg->scode, &msg->reason);
}


/* called when the session is established */
static void establish_handler(const struct sip_msg *msg, void *arg)
{
	(void)msg;
	(void)arg;

	re_printf("session established\n");
}



SIPUE::~SIPUE()
{
    mem_deref(sess_sock);
    mem_deref(reg);
}

void SIPUE::register_ue()
{
    my_sip = get_sip_stack();

    sipsess_listen(&sess_sock, my_sip, 32, static_connect_handler, this);
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

void SIPUE::call(std::string uri)
{
    const char* routes[1] = {_registrar.c_str()};
    struct mbuf *mb;
    sdp_session_alloc(&sdp, &laddr);
    sdp_media_add(&sdp_media, sdp, "audio", 4242, "RTP/AVP");
    sdp_format_add(NULL, sdp_media, false, "0", "PCMU", 8000, 1,
                   NULL, NULL, NULL, false, NULL);

    /* create SDP offer */
    sdp_encode(&mb, sdp, true);
    int err = sipsess_connect(&sess, sess_sock, uri.c_str(), "RKD",
                          _uri.c_str(), "RKD",
                          routes, 1, "application/sdp", mb,
                          static_auth_handler, this, false,
                          NULL, NULL,
                          progress_handler, establish_handler,
                          NULL, NULL, close_handler, NULL, NULL);
    printf("err: %d\n", err);
    mem_deref(mb); /* free SDP buffer */

}

/* called when register responses are received */
void SIPUE::register_handler(int err, const struct sip_msg *msg)
{
//    if (err)
//        re_printf("register error: %s\n", strerror(err));
    //else
    re_printf("register reply: %u %r\n", msg->scode, &msg->reason);
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


