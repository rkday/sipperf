/**
 * @file ack.c  SIP Session ACK
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_list.h>
#include <re_hash.h>
#include <re_fmt.h>
#include <re_uri.h>
#include <re_tmr.h>
#include <re_msg.h>
#include <re_sip.h>
#include <re_sipsess.h>
#include "sipsess.h"


struct sipsess_ack {
	struct le he;
	struct tmr tmr;
	struct sa dst;
	struct sip_request *req;
	struct sip_dialog *dlg;
    struct sip_auth *auth;
    struct mbuf *desc;
    const char *ctype;
	enum sip_transp tp;
	uint32_t cseq;
};


static void destructor(void *arg)
{
	struct sipsess_ack *ack = arg;

	hash_unlink(&ack->he);
	tmr_cancel(&ack->tmr);
//	mem_deref(ack->req);
	mem_deref(ack->dlg);
	mem_deref(ack->auth);
	mem_deref(ack->desc);
}


static void tmr_handler(void *arg)
{
	struct sipsess_ack *ack = arg;

	mem_deref(ack);
}


static int send_handler(enum sip_transp tp, const struct sa *src,
			const struct sa *dst, struct mbuf *mb, void *arg)
{
	return 0;
}


static void resp_handler(int err, const struct sip_msg *msg, void *arg)
{
	struct sipsess_ack *ack = arg;
	(void)err;
	(void)msg;

	//mem_deref(ack);
}

static int sipsess_send_ack(struct sipsess_sock *sock, struct sipsess_ack *ack)
{
    int err;

	err = sip_drequestf(NULL, sock->sip, false, "ACK", ack->dlg, ack->cseq,
			    ack->auth, send_handler, resp_handler, ack,
			    "%s%s%s"
			    "Content-Length: %zu\r\n"
			    "\r\n"
			    "%b",
			    ack->desc ? "Content-Type: " : "",
			    ack->desc ? ack->ctype : "",
			    ack->desc ? "\r\n" : "",
			    ack->desc ? mbuf_get_left(ack->desc) : (size_t)0,
			    ack->desc ? mbuf_buf(ack->desc) : NULL,
			    ack->desc ? mbuf_get_left(ack->desc) : (size_t)0);

    return err;
}

int sipsess_ack(struct sipsess_sock *sock, struct sip_dialog *dlg,
		uint32_t cseq, struct sip_auth *auth,
		const char *ctype, struct mbuf *desc)
{
	struct sipsess_ack *ack;
	int err;

	ack = mem_zalloc(sizeof(*ack), destructor);
	if (!ack)
		return ENOMEM;

	hash_append(sock->ht_ack,
		    hash_joaat_str(sip_dialog_callid(dlg)),
		    &ack->he, ack);

	ack->dlg  = mem_ref(dlg);
	ack->cseq = cseq;
	ack->auth = mem_ref(auth);
	ack->desc = mem_ref(desc);
	ack->ctype = ctype;

    err = sipsess_send_ack(sock, ack);
	tmr_start(&ack->tmr, 64 * SIP_T1, tmr_handler, ack);

	return err;
}


static bool cmp_handler(struct le *le, void *arg)
{
	struct sipsess_ack *ack = le->data;
	const struct sip_msg *msg = arg;

	if (!sip_dialog_cmp(ack->dlg, msg))
		return false;

	if (ack->cseq != msg->cseq.num)
		return false;

	return true;
}


int sipsess_ack_again(struct sipsess_sock *sock, const struct sip_msg *msg)
{
	struct sipsess_ack *ack;

	ack = list_ledata(hash_lookup(sock->ht_ack,
				      hash_joaat_pl(&msg->callid),
				      cmp_handler, (void *)msg));
	if (!ack)
		return ENOENT;

    int err = sipsess_send_ack(sock, ack);
    if (err)
        printf("sipsess_ack_again - sip_send returned %d\n", err);
    return err;
}
