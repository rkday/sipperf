/**
 * @file tmr.c  Timer implementation
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#endif
#ifdef HAVE_PTHREAD
#include <stdlib.h>
#include <pthread.h>
#endif
#include <re_types.h>
#include <re_fmt.h>
#include <re_list.h>
#include <re_mem.h>
#include <re_tmr.h>

#define DEBUG_MODULE "tmr"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


#ifndef RELEASE
#define TMR_DEBUG 1  /**< Timer debugging (0 or 1) */
#endif

/** Timer values */
enum {
	MAX_BLOCKING = 100   /**< Maximum time spent in handler [ms] */
};

extern heap_t *tmrh_get(void);

#if TMR_DEBUG
static void call_handler(tmr_h *th, void *arg)
{
	const uint64_t tick = tmr_jiffies();
	uint32_t diff;

	/* Call handler */
	th(arg);

	diff = (uint32_t)(tmr_jiffies() - tick);

	if (diff > MAX_BLOCKING) {
		DEBUG_WARNING("long async blocking: %u>%u ms (h=%p arg=%p)\n",
			      diff, MAX_BLOCKING, th, arg);
	}
}
#endif


/**
 * Poll all timers in the current thread
 *
 * @param tmrh Timer heap
 */
void tmr_poll(heap_t *tmrh)
{
	const uint64_t jfs = tmr_jiffies();

	for (;;) {
		struct tmr *tmr;
		tmr_h *th;
		void *th_arg;

		tmr = heap_peek(tmrh);

		if (!tmr || (tmr->jfs > jfs)) {
			break;
		}

		th = tmr->th;
		th_arg = tmr->arg;

		tmr->th = NULL;

		heap_remove_item(tmrh, tmr);

		if (!th)
			continue;

#if TMR_DEBUG
		call_handler(th, th_arg);
#else
		th(th_arg);
#endif
	}
}


/**
 * Get the timer jiffies in milliseconds
 *
 * @return Jiffies in [ms]
 */
uint64_t tmr_jiffies(void)
{
	uint64_t jfs;

#if defined(WIN32)
	FILETIME ft;
	ULARGE_INTEGER li;
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	jfs = li.QuadPart/10/1000;
#else
	struct timeval now;

	if (0 != gettimeofday(&now, NULL)) {
		DEBUG_WARNING("jiffies: gettimeofday() failed (%m)\n", errno);
		return 0;
	}

	jfs  = (long)now.tv_sec * (uint64_t)1000;
	jfs += now.tv_usec / 1000;
#endif

	return jfs;
}


/**
 * Get number of milliseconds until the next timer expires
 *
 * @param tmrh Timer heap
 *
 * @return Number of [ms], or 0 if no active timers
 */
uint64_t tmr_next_timeout(heap_t *tmrh)
{
	const uint64_t jif = tmr_jiffies();
	const struct tmr *tmr;

	tmr = heap_peek(tmrh);
	if (!tmr)
		return 0;

	if (tmr->jfs <= jif)
		return 1;
	else
		return tmr->jfs - jif;
}


int tmr_status(struct re_printf *pf, void *unused)
{
	(void)unused;
	heap_t *tmrh = tmrh_get();
	return heap_status(tmrh, pf);
}


/**
 * Print timer debug info to stderr
 */
void tmr_debug(void)
{
	if (heap_count(tmrh_get()) > 0)
		(void)re_fprintf(stderr, "%H", tmr_status, NULL);
}


/**
 * Initialise a timer object
 *
 * @param tmr Timer to initialise
 */
void tmr_init(struct tmr *tmr)
{
	if (!tmr)
		return;

	memset(tmr, 0, sizeof(*tmr));
	tmr->heap_idx = -1;
}


/**
 * Start a timer
 *
 * @param tmr   Timer to start
 * @param delay Timer delay in [ms]
 * @param th    Timeout handler
 * @param arg   Handler argument
 */
void tmr_start(struct tmr *tmr, uint64_t delay, tmr_h *th, void *arg)
{
	heap_t *tmrh = tmrh_get();

	if (!tmr)
		return;

	if (tmr->th) {
		heap_remove_item(tmrh, tmr);
	}

	tmr->th  = th;
	tmr->arg = arg;

	if (!th)
		return;

	tmr->jfs = delay + tmr_jiffies();

	heap_offer(tmrh, tmr);

#ifdef HAVE_ACTSCHED
	/* TODO: this is a hack. when a new timer is started we must reset
	   the main sleeping timer in actsched.cpp */
	{
		extern void actsched_restart_timer(void);
		actsched_restart_timer();
	}
#endif
}


/**
 * Cancel an active timer
 *
 * @param tmr Timer to cancel
 */
void tmr_cancel(struct tmr *tmr)
{
	tmr_start(tmr, 0, NULL, NULL);
}


/**
 * Get the time left until timer expires
 *
 * @param tmr Timer object
 *
 * @return Time in [ms] until expiration
 */
uint64_t tmr_get_expire(const struct tmr *tmr)
{
	uint64_t jfs;

	if (!tmr || !tmr->th)
		return 0;

	jfs = tmr_jiffies();

	return (tmr->jfs > jfs) ? (tmr->jfs - jfs) : 0;
}
