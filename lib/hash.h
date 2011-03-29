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

#ifndef HASH_H
#define HASH_H
#include "list.h"

#define HASH_STRUCT(type, name, elem) LIST_STRUCT(type, name, elem)

typedef list_elem_t hash_elem_t;

typedef unsigned (*hash_func_t)(hash_elem_t *elem);
typedef int      (*hash_comp_t)(hash_elem_t *e1, hash_elem_t *e2);

typedef struct {
    unsigned     size;
    list_t      *bins;
    hash_func_t  hash_func;
    hash_comp_t  comp_func;
} hash_t;

#define HASH_FOR(hash, iter)                            \
    for (unsigned __i = 0; __i < (hash)->size; __i++)   \
        LIST_FOR(&(hash)->bins[__i], iter)

#define HASH_FOR_S(hash, iter)                          \
    for (unsigned __i = 0; __i < (hash)->size; __i++)   \
        LIST_FOR_S(&(hash)->bins[__i], iter)

#define HASH_FOR_S_END LIST_FOR_S_END


int uart_sampler_hash_init(hash_t *hash, unsigned size,
			   hash_func_t hash_func,
			   hash_comp_t comp_func);

int uart_sampler_hash_fini(hash_t *hash);

void         uart_sampler_hash_insert(hash_t *hash, hash_elem_t *elem);
void         uart_sampler_hash_remove(hash_t *hash, hash_elem_t *elem);
hash_elem_t *uart_sampler_hash_lookup(hash_t *hash, hash_elem_t *elem);


#define hash_init uart_sampler_hash_init
#define hash_fini uart_sampler_hash_fini
#define hash_insert uart_sampler_hash_insert
#define hash_remove uart_sampler_hash_remove
#define hash_lookup uart_sampler_hash_lookup

#endif /* HASH_H */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */
