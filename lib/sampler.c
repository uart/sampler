/*
 * Copyright (C) 2009-2011, David Eklöv
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <assert.h>

#include "list.h"
#include "hash.h"
#include <uart/sampler.h>

#define HASH_BINS 1024

typedef struct {
    list_elem_t  elem;
    usf_file_t  *usf_file;
    char         name[256];
} burst_t;

typedef struct {
    hash_t          hash;
    list_t          list;

    burst_t        *burst;
    unsigned long   burst_idx;
} sampler_internal_t;

typedef struct {
    hash_elem_t   elem;
    unsigned      line;
    burst_t      *burst;
    usf_access_t  ref;
} watchpoint_t;

#define MAX(_a, _b) ({                          \
            __typeof__(_a) __a = _a;            \
            __typeof__(_b) __b = _b;            \
            __a < __b ? __b : __a;              \
        })

#define E_IF(_cond, _ret) do {                  \
        if (_cond) {                            \
            _LOG("error: %s", #_cond);          \
            return _ret;                        \
        }                                       \
    } while (0)
#define E_USF(error, _ret) E_IF((error) != USF_ERROR_OK, _ret) 

#define SAMPLE_RND(_s) ((_s)->sample_rnd((_s)->sample_period))
#define BURST_RND(_s)  ((_s)->burst_rnd((_s)->burst_period))


#ifdef DEBUG
static int log_level = 0;
#define _LOG(_fmt, _args...) do {                                       \
        printf("%s:%d: " _fmt "\n", __FUNCTION__, __LINE__, ##_args);   \
    } while (0)

#define LOG(_l, _fmt, _args...) do {            \
        if (_l <= log_level)                    \
            _LOG(_fmt, ##_args);                \
    } while (0)
#else
#define _LOG(_fmt, _args...)
#define LOG(_l, _fmt, _args...)
#endif


static burst_t *
burst_new(sampler_t *s, char *file_path, usf_atime_t begin_time)
{
    burst_t *burst;
    usf_header_t header;
    usf_error_t error;
    usf_event_t event;

    burst = (burst_t *)malloc(sizeof(burst_t));
    E_IF(burst == NULL, NULL);

    header.version = USF_VERSION_CURRENT;
    header.compression = USF_COMPRESSION_BZIP2;
    header.flags = s->usf_flags;
    header.time_begin = 0;
    header.time_end = 0;
    header.line_sizes = (1 << s->line_size_lg2);
    header.argc = 0;
    header.argv = NULL;

    error = usf_create(&burst->usf_file, file_path, &header);
    E_USF(error, NULL);

    event.type = USF_EVENT_BURST;
    event.u.burst.begin_time = begin_time;
    
    error = usf_append(burst->usf_file, &event);
    E_USF(error, NULL);
    return burst;
}

static int
burst_del(burst_t *burst)
{
    usf_error_t error;

    error = usf_close(burst->usf_file);
    E_IF(error != USF_ERROR_OK, -1);
    LOG(2, "burst: %s\n", burst->name);

    free(burst);
    return 0;
}

static int
burst_log_smpl(burst_t *burst, usf_access_t *ref1, usf_access_t *ref2,
	       usf_line_size_2_t line_size_lg2)
{
    usf_event_t event;
    usf_error_t error;

    event.type = USF_EVENT_SAMPLE; 
    event.u.sample.begin = *ref1;
    event.u.sample.end = *ref2;
    event.u.sample.line_size = line_size_lg2;

    error = usf_append(burst->usf_file, &event);
    E_USF(error, -1);
    LOG(2, "burst: %s\n", burst->name);
    return 0;
}

static int
burst_log_dngl(burst_t *burst, usf_access_t *ref,
	       usf_line_size_2_t line_size_lg2)
{
    usf_event_t event;
    usf_error_t error;

    event.type = USF_EVENT_DANGLING;
    event.u.dangling.begin = *ref;
    event.u.dangling.line_size = line_size_lg2;

    error = usf_append(burst->usf_file, &event);
    E_USF(error, -1);
    LOG(2, "burst: %s\n", burst->name);
    return 0;
}

static unsigned
watchpoint_hash_func(hash_elem_t *e)
{
    watchpoint_t *w = HASH_STRUCT(watchpoint_t, elem, e);
    return w->line & (HASH_BINS - 1);
}

static int
watchpoint_hash_comp(hash_elem_t *e1, hash_elem_t *e2)
{
    watchpoint_t *w1 = HASH_STRUCT(watchpoint_t, elem, e1);
    watchpoint_t *w2 = HASH_STRUCT(watchpoint_t, elem, e2);
    return w1->line == w2->line;
}

static int
watchpoint_insert(hash_t *hash, burst_t *burst, unsigned line, usf_access_t *ref)
{
    watchpoint_t *w;

    w = (watchpoint_t *)malloc(sizeof(watchpoint_t));
    E_IF(w == NULL, -1);

    w->line  =  line;
    w->burst =  burst;
    w->ref   = *ref;

    hash_insert(hash, &w->elem);
    return 0;
}

static watchpoint_t *
watchpoint_lookup(hash_t *hash, unsigned line)
{
    hash_elem_t  *e;
    watchpoint_t  w_cmp = { .line = line };

    e = hash_lookup(hash, &w_cmp.elem);
    if (!e)
        return NULL;
    hash_remove(hash, e);
    return HASH_STRUCT(watchpoint_t, elem, e);
}



int
sampler_init(sampler_t *s)
{
    int err;
    sampler_internal_t *internal;

    bzero(s, sizeof(sampler_t));

    internal = malloc(sizeof(sampler_internal_t));
    E_IF(!internal, -1);
    s->_internal = internal;

    s->usf_flags = USF_FLAG_NATIVE_ENDIAN | USF_FLAG_BURST;

    err = hash_init(&internal->hash, HASH_BINS, watchpoint_hash_func, watchpoint_hash_comp);
    E_IF(err, -1);

    err = list_init(&internal->list);
    E_IF(err, -1);

    return 0;
}

int
sampler_fini(sampler_t *s)
{
    int err;

    sampler_internal_t *internal = (sampler_internal_t *)s->_internal;
    hash_elem_t *iter_h;
    HASH_FOR_S(&internal->hash, iter_h) {
        watchpoint_t *w = HASH_STRUCT(watchpoint_t, elem, iter_h);

        err = burst_log_dngl(w->burst, &w->ref, s->line_size_lg2);
        E_IF(err, -1);

        hash_remove(&internal->hash, iter_h);
        free(w);
    } HASH_FOR_S_END;

    list_elem_t *iter_l;
    LIST_FOR_S(&internal->list, iter_l) {
        burst_t *b = LIST_STRUCT(burst_t, elem, iter_l);

        err = burst_del(b);
        E_IF(err, -1);
        
        //list_remove(iter_l);
    } LIST_FOR_S_END;

    free(s->_internal);
    s->_internal = NULL;

    return 0;
}

/*
 * Low level API
 */
int
sampler_watchpoint_lookup(sampler_t *s, usf_access_t *ref)
{
    sampler_internal_t *internal = (sampler_internal_t *)s->_internal;
    int           err;
    unsigned      line  = ref->addr >> s->line_size_lg2;
    watchpoint_t *w_hit = watchpoint_lookup(&internal->hash, line);

    if (w_hit) {
        err = burst_log_smpl(w_hit->burst, &w_hit->ref, ref, s->line_size_lg2);
        E_IF(err, -1);
        free(w_hit);
    }

    return 0;
}

int
sampler_watchpoint_insert(sampler_t *s, usf_access_t *ref)
{
    sampler_internal_t *internal = (sampler_internal_t *)s->_internal;
    int      err;
    unsigned line = ref->addr >> s->line_size_lg2;

    err = watchpoint_insert(&internal->hash, internal->burst, line, ref);
    E_IF(err, -1);

    return 0;
}

int
sampler_burst_begin(sampler_t *s, unsigned long time)
{
    sampler_internal_t *internal = (sampler_internal_t *)s->_internal;
    burst_t *burst;
    char     path[256];

    snprintf(path, 256, "%s.%lu", s->usf_base_path, internal->burst_idx++);

    burst = burst_new(s, path, time);
    E_IF(!burst, -1);

    strncpy(burst->name, path, 256);

    internal->burst = burst;
    list_push_back(&internal->list, &burst->elem);

    return 0;
}

int
sampler_burst_end(sampler_t *s, unsigned long time)
{
    sampler_internal_t *internal = (sampler_internal_t *)s->_internal;
    internal->burst = NULL;
    return 0;
}

int
sampler_burst_active(sampler_t *s)
{
    sampler_internal_t *internal = (sampler_internal_t *)s->_internal;
    return internal->burst != NULL;
}

/*
 * High level API
 */

unsigned
sampler_rnd_exp(unsigned period)
{
    double r = (double)rand() / RAND_MAX;
    return (unsigned)(period * -log(1 - r));
}

unsigned
sampler_rnd_const(unsigned period)
{
    return period;
}


int
sampler_ref(sampler_t *s, usf_access_t *ref)
{
    int           err;
    unsigned long time = ref->time;
    sampler_internal_t *internal = (sampler_internal_t *)s->_internal;

    err = sampler_watchpoint_lookup(s, ref);
    E_IF(err, -1);

    if (s->burst_size) {
        if (s->burst_end == time) {
            err = sampler_burst_end(s, time);
            E_IF(err, -1);

            s->burst_begin = time + BURST_RND(s);
        }

        if (s->burst_begin == time) {
            err = sampler_burst_begin(s, time);
            E_IF(err, -1);

            s->next_sample = time; // Always sample the first access in a burst.
            s->burst_end   = time + s->burst_size;
        }
    }

    if (internal->burst && s->next_sample == time) {
        err = sampler_watchpoint_insert(s, ref);
        E_IF(err, -1);
    
        s->next_sample = time + MAX(SAMPLE_RND(s), 1);
    }

    return 0;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */
