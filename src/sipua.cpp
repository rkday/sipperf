#include "sipua.hpp"
#include "uamanager.hpp"
#include "stats_displayer.hpp"
#include "stack.hpp"


uint64_t SIPUE::counter = 0;

SIPUE::~SIPUE() {
    mem_deref(reg);
}

//
// Registration methods.
//

void SIPUE::register_ue() {
    auto stackinfo = get_sip_stack();
    my_sip = stackinfo.first;
    my_sess_sock = stackinfo.second;

    sipreg_register(&reg,
                    my_sip,
                    _registrar.c_str(),
                    _uri.c_str(),
                    _uri.c_str(),
                    300,
                    _name.c_str(),
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

/* called when challenged for credentials */
int SIPUE::auth_handler(char **user, char **pass, const char *realm) {
    str_dup(user, _username.c_str());
    str_dup(pass, _password.c_str());

    return 0;
}

int SIPUE::static_auth_handler(char **user, char **pass, const char *realm, void *arg) {
    return ((SIPUE*)arg)->auth_handler(user, pass, realm);
}

/* called when register responses are received */
void SIPUE::register_handler(int err, const struct sip_msg *msg) {
    if (msg && msg->scode == 200) {
        stats_displayer->success_reg++;
        UAManager::get_instance()->mark_ua_registered(this);
    } else {
        stats_displayer->fail_reg++;
    }
}

void SIPUE::static_register_handler(int err, const struct sip_msg *msg, void *arg) {
    ((SIPUE*)arg)->register_handler(err, msg);
}

//
// Outgoing call methods
//

void SIPUE::call(std::string uri) {
    caller = true;
    // Use the registrar as an outbound proxy
    const char* routes[1] = {_registrar.c_str()};

    // Set up some dummy SDP to look realistic
    struct mbuf *mb;
    net_default_source_addr_get(AF_INET, &laddr);
    sa_set_port(&laddr, 0);
    sdp_session_alloc(&sdp, &laddr);
    sdp_media_add(&sdp_media, sdp, "audio", 4242, "RTP/AVP");
    sdp_format_add(NULL, sdp_media, false, "0", "PCMU", 8000, 1,
                   NULL, NULL, NULL, false, NULL);
    sdp_encode(&mb, sdp, true);

    UAManager::get_instance()->mark_ua_in_call(this);
    int err = sipsess_connect(&sess, my_sess_sock, uri.c_str(), _name.c_str(),
                              _uri.c_str(), _name.c_str(),
                              routes, 1, "application/sdp", mb,
                              static_auth_handler, this, false,
                              offer_handler, answer_handler,
                              static_progress_handler, static_establish_handler,
                              NULL, NULL, static_close_handler, this, NULL);

    mem_deref(mb);

}

/* called when SIP progress (like 180 Ringing) responses are received */
void SIPUE::progress_handler(const struct sip_msg *msg) {
}

void SIPUE::static_progress_handler(const struct sip_msg *msg, void *arg) {
    SIPUE* ue = static_cast<SIPUE*>(arg);
    ue->progress_handler(msg);
}

//
// Incoming call methods
//

void SIPUE::connect_handler(const struct sip_msg *msg) {
    caller = false;
    re_printf("connection attempt to %s\n", _uri.c_str());
    UAManager::get_instance()->mark_ua_in_call(this);
    //sip_treply(NULL, my_sip, msg, 486, "Busy Here");
    int err = sipsess_accept(&sess, my_sess_sock, msg, 180, "Ringing",
                             _name.c_str(), "application/sdp", NULL,
                             NULL, // auth_handler - we don't support non-REGISTER challenges
                             NULL, // authentication handler argument
                             false,
                             offer_handler, // offer handler
                             answer_handler, // answer handler
                             static_establish_handler,
                             NULL, // INFO handler
                             NULL, // REFER handler
                             static_close_handler,
                             this,
                             NULL); // extra headers
    sipsess_answer(sess, 200, "OK", msg->mb, "");
    return;
}


//
// Methods common to incoming and outgoing calls
//

void SIPUE::establish_handler(const struct sip_msg *msg) {
    //re_printf("session established for %s\n", ue->uri().c_str());
    stats_displayer->success_call++;
    if (caller) {
        struct sip_dialog* dlg = sipsess_dialog(sess);
        struct sip_request* bye;
        int err = sip_drequestf(NULL, my_sip, true, "BYE", dlg, 10, NULL, NULL, static_in_dialog_response_handler, this, "Content-Length: 0\r\n\r\n");
        UAManager::get_instance()->mark_ua_not_in_call(this);
        mem_deref(sess);
        mem_deref(sdp);
        sess = NULL;
        sdp = NULL;
        printf("err is %d\n", err);
    }
}

/* called when the session is established */
void SIPUE::static_establish_handler(const struct sip_msg *msg, void *arg) {
    SIPUE* ue = static_cast<SIPUE*>(arg);
    ue->establish_handler(msg);
}

/* called when the session fails to connect or is terminated from peer */
void SIPUE::close_handler(int err, const struct sip_msg *msg) {
    UAManager::get_instance()->mark_ua_not_in_call(this);
    if (err != ECONNRESET)
        stats_displayer->failed_call++;
//        re_printf("session closed for %s: %d\n", _uri.c_str(),  err);
 //   else
  //      re_printf("session closed: %u %r\n", msg->scode, &msg->reason);
    mem_deref(sess);
    sess = NULL;
}

void SIPUE::static_close_handler(int err, const struct sip_msg *msg, void *arg) {
    SIPUE* ue = static_cast<SIPUE*>(arg);
    ue->close_handler(err, msg);
}

void SIPUE::in_dialog_response_handler(int err, const struct sip_msg *msg) {

}

void SIPUE::static_in_dialog_response_handler(int err, const struct sip_msg *msg, void *arg) {
    SIPUE* ue = static_cast<SIPUE*>(arg);
    ue->in_dialog_response_handler(err, msg);
}



int SIPUE::answer_handler(const struct sip_msg *msg, void *arg) {
    return 0;
}

int SIPUE::offer_handler(mbuf** m, const struct sip_msg *msg, void *arg) {
    return 0;
}


