#pragma once

#include <stdint.h>
#include <unistd.h>
#include <re.h>
#include <string>

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

    ~SIPUE();

    void register_ue();
    void call(std::string uri);

private:
    void register_handler(int err, const struct sip_msg *msg);
    int auth_handler(char **user, char **pass, const char *realm);

    static void static_register_handler(int err, const struct sip_msg *msg, void* arg);
    static int static_auth_handler(char **user, char **pass, const char *realm, void* arg);

    struct sip *my_sip;            /* SIP session        */
    struct sipsess *sess;            /* SIP session        */
    struct sipsess_sock *sess_sock;  /* SIP session socket */
    struct sipreg *reg;              /* SIP registration   */
    struct sdp_session* sdp;
    struct sdp_media* sdp_media;
    struct sa laddr;

    std::string _registrar;
    std::string _uri;
    std::string _username;
    std::string _password;
};


