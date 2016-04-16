#include "sipua.hpp"
#include "stats_displayer.hpp"
#include "stack.hpp"

/* called upon incoming calls */
void SIPUE::static_connect_handler(const struct sip_msg *msg, void *arg)
{
    SIPUE* ue = static_cast<SIPUE*>(arg);
    ue->connect_handler(msg);
}
/* called when SIP progress (like 180 Ringing) responses are received */
static void progress_handler(const struct sip_msg *msg, void *arg)
{
	(void)arg;

	//re_printf("session progress: %u %r\n", msg->scode, &msg->reason);
}

/* called when the session fails to connect or is terminated from peer */
static void close_handler(int err, const struct sip_msg *msg, void *arg)
{
	SIPUE* ue = static_cast<SIPUE*>(arg);

	if (err)
		re_printf("session closed for %s: %d\n", ue->uri().c_str(),  err);
	else
		re_printf("session closed: %u %r\n", msg->scode, &msg->reason);
}


static int answer_handler(const struct sip_msg *msg, void *arg)
{
    return 0;
}

static int offer_handler(mbuf** m, const struct sip_msg *msg, void *arg)
{
    return 0;
}

/* called when the session is established */
static void static_establish_handler(const struct sip_msg *msg, void *arg)
{
	SIPUE* ue = static_cast<SIPUE*>(arg);
    ue->establish_handler(msg);
}

int bye_send_handler(enum sip_transp tp, const struct sa *src,
                     const struct sa *dst, struct mbuf *mb, void *arg)
{
    printf("Sending %d-character BYE to %s...\n", mb->size, inet_ntoa(dst->u.in.sin_addr));
    /*
    char buf[2048] = {0};
    mb->pos = 0;
    mbuf_read_str(mb, buf, 300);
    printf("%s\n", buf);
    */
    return 0;
}

void SIPUE::establish_handler(const struct sip_msg *msg)
{
	//re_printf("session established for %s\n", ue->uri().c_str());
    stats_displayer->success_call++;
    struct sip_dialog* dlg = sipsess_dialog(sess);
    struct sip_request* bye;
    int err = sip_drequestf(&bye, my_sip, false, "BYE", dlg, 10, NULL, bye_send_handler, NULL, this, "Content-Length: 0\r\n\r\n");
    printf("err is %d\n", err);
}


void SIPUE::connect_handler(const struct sip_msg *msg)
{
	//re_printf("connection attempt to %s\n", _uri.c_str());
    //(void)sip_treply(NULL, my_sip, msg, 486, "Busy Here");
	int err = sipsess_accept(&sess, sess_sock, msg, 200, "OK",
			     "RKD", "application/sdp", msg->mb,
			     NULL, // auth_handler - we don't support non-REGISTER challenges
                 NULL, // authentication handler argument
                 false,
			     offer_handler, // offer handler
                 answer_handler, // answer handler
			     static_establish_handler,
                 NULL, // INFO handler
                 NULL, // REFER handler
			     close_handler,
                 this,
                 NULL); // extra headers
    return;
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
                    30,
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
                          offer_handler, answer_handler,
                          progress_handler, static_establish_handler,
                          NULL, NULL, close_handler, this, NULL);
    mem_deref(mb); /* free SDP buffer */

}

/* called when register responses are received */
void SIPUE::register_handler(int err, const struct sip_msg *msg)
{
    if (msg->scode == 200)
    {
        stats_displayer->success_reg++;
    }
    else
    {
        stats_displayer->fail_reg++;
    }
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


