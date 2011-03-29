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

#ifndef UART_SAMPLER_H
#define UART_SAMPLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <uart/usf.h>

typedef struct {
    char           *usf_base_path;
    usf_flags_t     usf_flags;

    void *_internal;

    /* High level API */
    unsigned long   burst_begin;
    unsigned long   burst_end;
    unsigned long   next_sample;
    
    unsigned long   burst_period;
    unsigned 	   (*burst_rnd)(unsigned);

    unsigned long   sample_period;
    unsigned 	   (*sample_rnd)(unsigned);
    
    unsigned long   burst_size;
    unsigned short  line_size_lg2;

    int             log_level;
    unsigned        seed;
} sampler_t;


extern int sampler_init(sampler_t *s);
extern int sampler_fini(sampler_t *s);

/* Low level API */
extern int sampler_watchpoint_lookup(sampler_t *s, usf_access_t *ref);
extern int sampler_watchpoint_insert(sampler_t *s, usf_access_t *ref);

extern int sampler_burst_begin(sampler_t *s, unsigned long time);
extern int sampler_burst_end(sampler_t *s, unsigned long time);
extern int sampler_burst_active(sampler_t *s);

/* High level API */
extern unsigned sampler_rnd_exp(unsigned period);
extern unsigned sampler_rnd_const(unsigned period);

extern int sampler_ref(sampler_t *s, usf_access_t *ref);

#ifdef __cplusplus
}
#endif

#endif /* UART_SAMPLER_H */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */
