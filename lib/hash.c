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
#include "list.h"
#include "hash.h"

int
uart_sampler_hash_init(hash_t      *hash,
                       unsigned     size,
                       hash_func_t  hash_func,
                       hash_comp_t  comp_func)
{
    hash->bins = (list_t *)malloc(size * sizeof(list_t));
    if (!hash->bins)
        return 1;
    
    for (int i = 0; i < size; i++)
        list_init(&hash->bins[i]);

    hash->size = size;
    hash->hash_func = hash_func;
    hash->comp_func = comp_func;
    return 0;
}

int
uart_sampler_hash_fini(hash_t *hash)
{
    free(hash->bins);
    return 0;
}

void
uart_sampler_hash_insert(hash_t *hash, hash_elem_t *elem)
{
    list_t *bin = &hash->bins[hash->hash_func(elem)];

    list_push_front(bin, elem);
}

void
hash_remove(hash_t *hash, hash_elem_t *elem)
{
    list_remove(elem);
}

hash_elem_t *
uart_sampler_hash_lookup(hash_t *hash, hash_elem_t *elem)
{
    list_t *bin = &hash->bins[hash->hash_func(elem)];

    list_elem_t *iter;
    LIST_FOR(bin, iter) {
        if (hash->comp_func(iter, elem))
            return iter;
    }
    return NULL;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */
