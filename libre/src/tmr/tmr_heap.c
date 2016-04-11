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



#include <re_tmr.h>

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <assert.h>


#define DEFAULT_CAPACITY 13

typedef struct tmr* heap_entry;

struct heap_s
{
    /* size of array */
    unsigned int size;
    /* items within heap */
    unsigned int count;
    heap_entry* array;
};

static int keycompare(uint64_t a, uint64_t b)
{
    if (a < b)
    {
        return 1;
    }
    else if (a > b)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static int __child_left(const int idx)
{
    return idx * 2 + 1;
}

static int __child_right(const int idx)
{
    return idx * 2 + 2;
}

static int __parent(const int idx)
{
    return (idx - 1) / 2;
}

void dump_heap(const heap_t* h)
{
    printf("===\n");
    for (int ii = 0; ii < h->count; ii++)
    {
        printf("Index %d (%d): parent %d, pointer %p, timeout %lu\n", ii, h->array[ii]->heap_idx, __parent(ii), h->array[ii], h->array[ii]->jfs);
    }
    printf("===\n");
}

void check_heap(const heap_t* h)
{
#if 0
    dump_heap(h);
    for (int ii = 1; ii < h->count; ii++)
    {
        uint64_t thiskey = h->array[ii]->jfs;
        uint64_t parentkey = h->array[__parent(ii)]->jfs;
        assert(parentkey <= thiskey);
    }
#endif
}



void heap_init(heap_t* h,
               unsigned int size
               )
{
    h->size = size;
    h->count = 0;
}

heap_t *heap_new()
{
    heap_t *h = malloc(sizeof(heap_t));
    h->array = malloc(DEFAULT_CAPACITY * sizeof(heap_entry));

    if (!h)
        return NULL;

    heap_init(h, DEFAULT_CAPACITY);

    return h;
}

void heap_free(heap_t * h)
{
    free(h->array);
    free(h);
}

/**
 * @return a new heap on success; NULL otherwise */
static int __ensurecapacity(heap_t * h)
{
    if (h->count < h->size)
        return 0;

    h->size *= 2;

    void* new_array = realloc(h->array, (h->size * sizeof(heap_entry)));
    
    if (new_array == NULL)
    {
        return 1;
    }
    else
    {
        h->array = new_array;
        return 0;
    }
}

static void __swap(heap_t * h, const int i1, const int i2)
{
    //printf("Swapping heap entries %d and %d\n", i1, i2);
    heap_entry tmp = h->array[i1];

    h->array[i1] = h->array[i2];
    h->array[i1]->heap_idx = i1;
    h->array[i2] = tmp;
    h->array[i2]->heap_idx = i2;
}

static int __pushup(heap_t * h, unsigned int idx)
{
    /* 0 is the root node */
    while (0 != idx)
    {
        int parent = __parent(idx);

        /* we are smaller than the parent */
        if (keycompare(h->array[idx]->jfs, h->array[parent]->jfs) < 0)
            return -1;
        else
            __swap(h, idx, parent);

        idx = parent;
    }

    return idx;
}

static void __pushdown(heap_t * h, unsigned int idx)
{
    while (1)
    {
        unsigned int childl, childr, child;

        childl = __child_left(idx);
        childr = __child_right(idx);

        if (childr >= h->count)
        {
            /* can't pushdown any further */
            if (childl >= h->count)
                return;

            child = childl;
        }
        /* find biggest child */
        else if (keycompare(h->array[childl]->jfs, h->array[childr]->jfs) < 0)
            child = childr;
        else
            child = childl;

        /* idx is smaller than child */
        if (keycompare(h->array[idx]->jfs, h->array[child]->jfs) < 0)
        {
            __swap(h, idx, child);
            idx = child;
            /* bigger than the biggest child, we stop, we win */
        }
        else
            return;
    }
}

static void __heap_offerx(heap_t * h, struct tmr* item)
{
    item->heap_idx = h->count;
    h->array[h->count] = item;

    /* ensure heap properties */
    __pushup(h, h->count++);
    check_heap(h);
}

int heap_offerx(heap_t * h, struct tmr *item)
{
    if (h->count == h->size)
        return -1;
    __heap_offerx(h, item);
    return 0;
}

int heap_offer(heap_t * h, struct tmr *item)
{
    if (__ensurecapacity(h) != 0)
        return -1;

    __heap_offerx(h, item);
    return 0;
}

struct tmr *heap_poll(heap_t * h)
{
    if (0 == heap_count(h))
        return NULL;

    heap_entry item = h->array[0];
    item->heap_idx = -1;

    h->array[0] = h->array[h->count - 1];
    h->array[0]->heap_idx = 0;
    h->count--;

    if (h->count > 1)
        __pushdown(h, 0);

    check_heap(h);
    return item;
}

struct tmr *heap_peek(const heap_t * h)
{
    if (0 == heap_count(h))
        return NULL;

    return h->array[0];
}

void heap_clear(heap_t * h)
{
    h->count = 0;
}

void heap_remove_item(heap_t * h, struct tmr *item)
{
    int idx = item->heap_idx;
    item->heap_idx = -1;

    if (idx == -1)
        return;

    /* swap the item we found with the last item on the heap */
    h->array[idx] = h->array[h->count - 1];
    h->array[idx]->heap_idx = idx;
    h->array[h->count - 1] = (struct tmr*)0xDEADBEEF;

    h->count -= 1;

    /* ensure heap property - tis is unnecessary if this was already bottom of the heap*/
    int parent_idx = __parent(idx);

    if ((unsigned int)idx != h->count)
    {
        if (h->array[idx]->jfs >= h->array[parent_idx]->jfs)
        {
                __pushdown(h, idx);
        }
        else
        {
                __pushup(h, idx);
        }
    }
    check_heap(h);
}

int heap_contains_item(const heap_t * h, const struct tmr *item)
{
    return item->heap_idx != -1;
}

int heap_count(const heap_t * h)
{
    return h->count;
}

int heap_size(const heap_t * h)
{
    return h->size;
}

/*--------------------------------------------------------------79-characters-*/
