/**
 * @file re_tmr.h  Interface to timer implementation
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <re_types.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct heap_s heap_t;
struct re_printf;

/**
 * Defines the timeout handler
 *
 * @param arg Handler argument
 */
typedef void (tmr_h)(void *arg);

/** Defines a timer */
struct tmr {
	tmr_h *th;          /**< Timeout handler     */
	void *arg;          /**< Handler argument    */
	uint64_t jfs;       /**< Jiffies for timeout */
    int heap_idx;
};


void     tmr_poll(heap_t *tmrh);
uint64_t tmr_jiffies(void);
uint64_t tmr_next_timeout(heap_t *tmrh);
void     tmr_debug(void);
int      tmr_status(struct re_printf *pf, void *unused);

void     tmr_init(struct tmr *tmr);
void     tmr_start(struct tmr *tmr, uint64_t delay, tmr_h *th, void *arg);
void     tmr_cancel(struct tmr *tmr);
uint64_t tmr_get_expire(const struct tmr *tmr);


/**
 * Check if the timer is running
 *
 * @param tmr Timer to check
 *
 * @return true if running, false if not running
 */
static inline bool tmr_isrunning(const struct tmr *tmr)
{
	return tmr ? NULL != tmr->th : false;
}

/* Timer heap methods - originally from https://github.com/willemt/heap
 
Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

/**
 * Create new heap and initialise it.
 *
 * malloc()s space for heap.
 *
 * @param[in] cmp Callback used to get an item's priority
 * @param[in] udata User data passed through to cmp callback
 * @return initialised heap */
heap_t *heap_new(void);

/**
 * Initialise heap. Use memory passed by user.
 *
 * No malloc()s are performed.
 *
 * @param[in] cmp Callback used to get an item's priority
 * @param[in] udata User data passed through to cmp callback
 * @param[in] size Initial size of the heap's array */
void heap_init(heap_t* h,
               unsigned int size);

void heap_free(heap_t * hp);

/**
 * Add item
 *
 * Ensures that the data structure can hold the item.
 *
 * NOTE:
 *  realloc() possibly called.
 *  The heap pointer will be changed if the heap needs to be enlarged.
 *
 * @param[in/out] hp_ptr Pointer to the heap. Changed when heap is enlarged.
 * @param[in] item The item to be added
 * @return 0 on success; -1 on failure */
int heap_offer(heap_t *hp, struct tmr* item);

/**
 * Add item
 *
 * An error will occur if there isn't enough space for this item.
 *
 * NOTE:
 *  no malloc()s called.
 *
 * @param[in] item The item to be added
 * @return 0 on success; -1 on error */
int heap_offerx(heap_t * hp, struct tmr *item);

/**
 * Remove the item with the top priority
 *
 * @return top item */
struct tmr *heap_poll(heap_t * hp);

/**
 * @return top item of the heap */
struct tmr *heap_peek(const heap_t * hp);

/**
 * Clear all items
 *
 * NOTE:
 *  Does not free items.
 *  Only use if item memory is managed outside of heap */
void heap_clear(heap_t * hp);

/**
 * @return number of items in heap */
int heap_count(const heap_t * hp);

/**
 * @return size of array */
int heap_size(const heap_t * hp);

/**
 * @return number of bytes needed for a heap of this size. */
size_t heap_sizeof(unsigned int size);

/**
 * Remove item
 *
 * @param[in] item The item that is to be removed
 * @return item to be removed; NULL if item does not exist */
void heap_remove_item(heap_t * hp, struct tmr *item);

/**
 * Test membership of item
 *
 * @param[in] item The item to test
 * @return 1 if the heap contains this item; otherwise 0 */
int heap_contains_item(const heap_t * hp, const struct tmr *item);


int heap_status(heap_t* tmrh, struct re_printf *pf);
