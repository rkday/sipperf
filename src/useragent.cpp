#include "useragent.hpp"
#include "uamanager.hpp"
#include "stats_displayer.hpp"
#include "stack.hpp"
#include "easylogging++.h"

#define RE_ERRCHECK(X) {int err = X; if (err) {LOG(WARNING) << "libre library call failed - error was" << err << " (" << strerror(err) << ") on line " << __LINE__;}};

uint64_t UserAgent::counter = 0;

UserAgent::~UserAgent() {
    mem_deref(reg);
}

std::string UserAgent::get_cid() {
    if (sess == nullptr)
    {
        return "";
    }
    struct sip_dialog* dlg = sipsess_dialog(sess);
    return sip_dialog_callid(dlg);
}

//
// Registration methods.
//

void UserAgent::register_ue() {
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
int UserAgent::auth_handler(char **user, char **pass, const char *realm) {
    str_dup(user, _username.c_str());
    str_dup(pass, _password.c_str());

    return 0;
}

int UserAgent::static_auth_handler(char **user, char **pass, const char *realm, void *arg) {
    return ((UserAgent*)arg)->auth_handler(user, pass, realm);
}

/* called when register responses are received */
void UserAgent::register_handler(int err, const struct sip_msg *msg) {
    if (msg && msg->scode == 200) {
        stats_displayer->success_reg++;
        UAManager::get_instance()->mark_ua_registered(this);
    } else {
        stats_displayer->fail_reg++;
        if (msg) {
            LOG(WARNING) << "Registration failed for "<< _uri << " with SIP error code " << msg->scode;
        } else {
            LOG(WARNING) << "Registration failed for "<< _uri << " with error " << err << " (" << strerror(err) << ")";
        }
    }
}

void UserAgent::static_register_handler(int err, const struct sip_msg *msg, void *arg) {
    ((UserAgent*)arg)->register_handler(err, msg);
}

//
// Outgoing call methods
//

void UserAgent::call(std::string uri) {
    stats_displayer->init_call++;
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
    RE_ERRCHECK(sipsess_connect(&sess, my_sess_sock, uri.c_str(), _name.c_str(),
                              _uri.c_str(), _name.c_str(),
                              routes, 1, "application/sdp", mb,
                              static_auth_handler, this, false,
                              offer_handler, answer_handler,
                              static_progress_handler, static_establish_handler,
                              NULL, NULL, static_close_handler, this, NULL));

    mem_deref(mb);

}

/* called when SIP progress (like 180 Ringing) responses are received */
void UserAgent::progress_handler(const struct sip_msg *msg) {
}

void UserAgent::static_progress_handler(const struct sip_msg *msg, void *arg) {
    UserAgent* ue = static_cast<UserAgent*>(arg);
    ue->progress_handler(msg);
}

//
// Incoming call methods
//

void UserAgent::connect_handler(const struct sip_msg *msg) {
    caller = false;
    UAManager::get_instance()->mark_ua_in_call(this);
    //sip_treply(NULL, my_sip, msg, 486, "Busy Here");
    RE_ERRCHECK(sipsess_accept(&sess, my_sess_sock, msg, 180, "Ringing",
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
                             NULL)); // extra headers
    RE_ERRCHECK(sipsess_answer(sess, 200, "OK", msg->mb, ""));
    return;
}


//
// Methods common to incoming and outgoing calls
//

void UserAgent::establish_handler(const struct sip_msg *msg) {
    if (caller) {
        stats_displayer->success_call++;
        struct sip_dialog* dlg = sipsess_dialog(sess);
        RE_ERRCHECK(sip_drequestf(NULL, my_sip, true, "BYE", dlg, 10, NULL, NULL, static_in_dialog_response_handler, this, "Content-Length: 0\r\n\r\n"));
        UAManager::get_instance()->mark_ua_not_in_call(this);
        mem_deref(sess);
        mem_deref(sdp);
        sess = NULL;
        sdp = NULL;
    }
}

/* called when the session is established */
void UserAgent::static_establish_handler(const struct sip_msg *msg, void *arg) {
    UserAgent* ue = static_cast<UserAgent*>(arg);
    ue->establish_handler(msg);
}

/* called when the session fails to connect or is terminated from peer */
void UserAgent::close_handler(int err, const struct sip_msg *msg) {
    UAManager::get_instance()->mark_ua_not_in_call(this);
    if (err != ECONNRESET) {
        stats_displayer->failed_call++;
        if (msg) {
            LOG(WARNING) << "Call " << get_cid() << " ended for "<< _uri << " with SIP error code " << msg->scode;
        } else {
            LOG(WARNING) << "Call " << get_cid() << " ended for "<< _uri << " with error " << err << " (" << strerror(err) << ")";
        }
    }
    mem_deref(sess);
    sess = NULL;
}

void UserAgent::static_close_handler(int err, const struct sip_msg *msg, void *arg) {
    UserAgent* ue = static_cast<UserAgent*>(arg);
    ue->close_handler(err, msg);
}

void UserAgent::in_dialog_response_handler(int err, const struct sip_msg *msg) {

}

void UserAgent::static_in_dialog_response_handler(int err, const struct sip_msg *msg, void *arg) {
    UserAgent* ue = static_cast<UserAgent*>(arg);
    ue->in_dialog_response_handler(err, msg);
}



int UserAgent::answer_handler(const struct sip_msg *msg, void *arg) {
    return 0;
}

int UserAgent::offer_handler(mbuf** m, const struct sip_msg *msg, void *arg) {
    return 0;
}


