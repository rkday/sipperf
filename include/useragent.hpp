#pragma once

#include <string>
#include <stdint.h>
#include <unistd.h>
#include <re.h>

class UserAgent
{
public:
    UserAgent(std::string registrar,
          std::string uri,
          std::string username,
          std::string password):
        _registrar(registrar),
        _uri(uri),
        _username(username),
        _password(password),
        _name(std::to_string(getpid()) + "-ue" + std::to_string(counter++))
    {}

    void register_ue();
    void unregister();
    void call(std::string uri);

    std::string uri() { return _uri; };
    std::string name() { return _name; }
    std::string get_cid();

    void establish_handler(const struct sip_msg *msg);
    void connect_handler(const struct sip_msg *msg);
private:
    void register_handler(int err, const struct sip_msg *msg);
    int auth_handler(char **user, char **pass, const char *realm);
    void progress_handler(const struct sip_msg *msg);
    void close_handler(int err, const struct sip_msg *msg);
    void in_dialog_response_handler(int err, const struct sip_msg *msg);

    static void static_progress_handler(const struct sip_msg *msg, void *arg);
    static void static_establish_handler(const struct sip_msg *msg, void *arg);
    static void static_register_handler(int err, const struct sip_msg *msg, void* arg);
    static int static_auth_handler(char **user, char **pass, const char *realm, void* arg);
    static void static_connect_handler(const struct sip_msg *msg, void *arg);
    static void static_close_handler(int err, const struct sip_msg *msg, void *arg);
    static void static_in_dialog_response_handler(int err, const struct sip_msg *msg, void *arg);

    static int answer_handler(const struct sip_msg *msg, void *arg);
    static int offer_handler(mbuf** m, const struct sip_msg *msg, void *arg);

    struct sip *my_sip = nullptr;            /* SIP session        */
    struct sipsess_sock *my_sess_sock = nullptr;
    struct sipsess *sess = nullptr;            /* SIP session        */
    struct sipreg *reg = nullptr;              /* SIP registration   */
    struct sdp_session* sdp = nullptr;
    struct sdp_media* sdp_media = nullptr;
    struct sa laddr;

    std::string _registrar;
    std::string _uri;
    std::string _username;
    std::string _password;
    std::string _name;
    bool caller;

    static uint64_t counter;
};


