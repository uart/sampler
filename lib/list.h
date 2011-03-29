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

#ifndef LIST_H
#define LIST_H
#include <stddef.h>

typedef struct list_elem {
    struct list_elem *next;
    struct list_elem *prev;
} list_elem_t;

typedef struct {
    list_elem_t head;
    list_elem_t tail;
} list_t;

#define LIST_BEGIN(list)  list_head(list)
#define LIST_RBEGIN(list) list_tail(list)

#define LIST_NEXT(iter)  (iter)->next
#define LIST_RNEXT(iter) (iter)->prev

#define LIST_END(list)  &(list)->tail
#define LIST_REND(list) &(list)->head

#define LIST_FOR(list, iter)                    \
    for (iter = LIST_BEGIN(list);               \
         iter != LIST_END(list);                \
         iter = LIST_NEXT(iter))

#define LIST_FOR_R(list, iter)                  \
    for (iter = LIST_RBEGIN(list);              \
         iter != LIST_REND(list);               \
         iter = LIST_RNEXT(iter))

#define LIST_FOR_S(list, iter) do {             \
    list_elem_t *_safe = LIST_BEGIN(list);      \
    while (_safe != LIST_END(list)) {           \
        iter = _safe;                           \
        _safe = LIST_NEXT(_safe);               \

#define LIST_FOR_S_END }} while (0);
        
#define LIST_STRUCT(type, name, elem)   \
    ((type *)(elem - offsetof(type, name)))


static inline int
list_init(list_t *list)
{
    list->head.next = &list->tail;
    list->head.prev = NULL;

    list->tail.next = NULL;
    list->tail.prev = &list->head;
    return 0;
}

static inline list_elem_t *
list_head(list_t *list)
{
    return list->head.next;
}

static inline list_elem_t *
list_tail(list_t *list)
{
    return list->tail.prev;
}

static inline int
list_empty(list_t *list)
{
    return list_head(list) == &list->tail &&
           list_tail(list) == &list->head;
}

static inline void
list_insert_before(list_elem_t *e1, list_elem_t *e2)
{
    e2->prev = e1->prev;
    e2->next = e1;
    e1->prev->next = e2;
    e1->prev = e2;
}

static inline void
list_insert_after(list_elem_t *e1, list_elem_t *e2)
{
    list_insert_before(e1->next, e2);
}

static inline void
list_remove(list_elem_t *e)
{
    e->prev->next = e->next;
    e->next->prev = e->prev;
}

static inline void
list_push_front(list_t *list, list_elem_t *e)
{
    list_insert_after(&list->head, e);
}

static inline void
list_push_back(list_t *list, list_elem_t *e)
{
    list_insert_before(&list->tail, e);
}

static inline list_elem_t *
list_pop_front(list_t *list)
{
    list_elem_t *e = NULL;
    if (!list_empty(list)) {
        e = list_head(list);
        list_remove(e);
    }
    return e;
}

static inline list_elem_t *
list_pop_back(list_t *list)
{
    list_elem_t *e = NULL;
    if (!list_empty(list)) {
        e = list_tail(list);
        list_remove(e);
    }
    return e;
}

#endif /* LIST_H */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */
