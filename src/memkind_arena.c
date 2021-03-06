/*
 * Copyright (C) 2014, 2015 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <jemalloc/jemalloc.h>
#include <utmpx.h>
#include <sched.h>
#include <smmintrin.h>

#include "config_tls.h"
#include "memkind.h"
#include "memkind_default.h"
#include "memkind_arena.h"

static void *je_mallocx_check(size_t size, int flags);
static void *je_rallocx_check(void *ptr, size_t size, int flags);

int memkind_arena_create(struct memkind *kind, const struct memkind_ops *ops, const char *name)
{
    int err = 0;

    err = memkind_default_create(kind, ops, name);
    if (!err) {
        err = memkind_arena_create_map(kind);
    }
    return err;
}

int memkind_arena_create_map(struct memkind *kind)
{
    int err = 0;
    int i;
    size_t unsigned_size = sizeof(unsigned int);

    if (kind->arena_map_len == 0) {
        if (kind->ops->get_arena == memkind_bijective_get_arena) {
            kind->arena_map_len = 1;
        }
        else if (kind->ops->get_arena == memkind_thread_get_arena) {
            kind->arena_map_len = numa_num_configured_cpus() * 4;
        }
    }
#ifndef MEMKIND_TLS
    if (kind->ops->get_arena == memkind_thread_get_arena) {
        pthread_key_create(&(kind->arena_key), je_free);
    }
#endif

    if (kind->arena_map_len) {
        kind->arena_map = (unsigned int *)je_malloc(sizeof(unsigned int) * kind->arena_map_len);
        if (kind->arena_map == NULL) {
            err = MEMKIND_ERROR_MALLOC;
        }
    }
    if (!err) {
        for (i = 0; i < kind->arena_map_len; ++i) {
            kind->arena_map[i] = UINT_MAX;
        }
        for (i = 0; !err && i < kind->arena_map_len; ++i) {
            err = je_mallctl("arenas.extendk", kind->arena_map + i,
                             &unsigned_size, &(kind->partition),
                             unsigned_size);
        }
        if (err) {
            if (kind->arena_map) {
                je_free(kind->arena_map);
            }
            err = MEMKIND_ERROR_MALLCTL;
        }
    }
    return err;
}

int memkind_arena_destroy(struct memkind *kind)
{
    char cmd[128];
    int i;

    if (kind->arena_map) {
        for (i = 0; i < kind->arena_map_len; ++i) {
            snprintf(cmd, 128, "arena.%u.purge", kind->arena_map[i]);
            je_mallctl(cmd, NULL, NULL, NULL, 0);
        }
        je_free(kind->arena_map);
        kind->arena_map = NULL;
    }
#ifndef MEMKIND_TLS
    if (kind->ops->get_arena == memkind_thread_get_arena) {
        pthread_key_delete(kind->arena_key);
    }
#endif
    memkind_default_destroy(kind);
    return 0;
}

void *memkind_arena_malloc(struct memkind *kind, size_t size)
{
    void *result = NULL;
    int err = 0;
    unsigned int arena;

    if (!err) {
        err = kind->ops->get_arena(kind, &arena);
    }
    if (!err) {
        result = je_mallocx_check(size, MALLOCX_ARENA(arena));
    }
    return result;
}

void *memkind_arena_realloc(struct memkind *kind, void *ptr, size_t size)
{
    int err = 0;
    unsigned int arena;

    if (size == 0 && ptr != NULL) {
        memkind_free(kind, ptr);
        ptr = NULL;
    }
    else {
        err = kind->ops->get_arena(kind, &arena);
        if (!err) {
            if (ptr == NULL) {
                ptr = je_mallocx_check(size, MALLOCX_ARENA(arena));
            }
            else {
                ptr = je_rallocx_check(ptr, size, MALLOCX_ARENA(arena));
            }
        }
    }
    return ptr;
}

void *memkind_arena_calloc(struct memkind *kind, size_t num, size_t size)
{
    void *result = NULL;
    int err = 0;
    unsigned int arena;

    err = kind->ops->get_arena(kind, &arena);
    if (!err) {
        result = je_mallocx_check(num * size, MALLOCX_ARENA(arena) | MALLOCX_ZERO);
    }
    return result;
}

int memkind_arena_posix_memalign(struct memkind *kind, void **memptr, size_t alignment,
                                 size_t size)
{
    int err = 0;
    unsigned int arena;
    int errno_before;

    *memptr = NULL;
    err = kind->ops->get_arena(kind, &arena);
    if (!err) {
        err = memkind_posix_check_alignment(kind, alignment);
    }
    if (!err) {
        /* posix_memalign should not change errno.
           Set it to its previous value after calling jemalloc */
        errno_before = errno;
        *memptr = je_mallocx_check(size, MALLOCX_ALIGN(alignment) | MALLOCX_ARENA(arena));
        errno = errno_before;
        err = *memptr ? 0 : ENOMEM;
    }
    return err;
}

int memkind_bijective_get_arena(struct memkind *kind, unsigned int *arena)
{
    int err = 0;

    if (kind->arena_map != NULL) {
        *arena = *(kind->arena_map);
        if (*arena == UINT_MAX) {
            err = MEMKIND_ERROR_MALLCTL;
        }
    }
    else {
        err = MEMKIND_ERROR_RUNTIME;
    }
    return err;
}

#ifdef MEMKIND_TLS
int memkind_thread_get_arena(struct memkind *kind, unsigned int *arena)
{
    static __thread unsigned int MEMKIND_TLS_MODEL arena_tls = UINT_MAX;

    if (arena_tls == UINT_MAX) {
        if (kind->arena_map != NULL) {
            arena_tls = _mm_crc32_u64(0, (uint64_t)pthread_self());
        }
    }
    *arena = kind->arena_map[arena_tls % kind->arena_map_len];
    return 0;
}
#else
int memkind_thread_get_arena(struct memkind *kind, unsigned int *arena)
{
    int err = 0;
    unsigned int *arena_tsd;

    arena_tsd = pthread_getspecific(kind->arena_key);
    if (arena_tsd == NULL) {
        arena_tsd = je_malloc(sizeof(unsigned int));
        if (arena_tsd == NULL) {
            err = MEMKIND_ERROR_MALLOC;
        }
        if (!err) {
            *arena_tsd = _mm_crc32_u64(0, (uint64_t)pthread_self()) %
                         kind->arena_map_len;
            err = pthread_setspecific(kind->arena_key, arena_tsd) ?
                  MEMKIND_ERROR_PTHREAD : 0;
        }
    }
    if (!err) {
        *arena = kind->arena_map[*arena_tsd];
        if (*arena == UINT_MAX) {
            err = MEMKIND_ERROR_MALLCTL;
        }
    }
    return err;
}
#endif /* MEMKIND_TLS */

static void *je_mallocx_check(size_t size, int flags)
{
    /*
     * Checking for out of range size due to unhandled error in
     * je_mallocx().  Size invalid for the range
     * LLONG_MAX <= size <= ULLONG_MAX
     * which is the result of passing a negative signed number as size
     */
    void *result = NULL;

    if (size >= LLONG_MAX) {
        errno = ENOMEM;
    }
    else {
        result = je_mallocx(size, flags);
    }
    return result;
}

static void *je_rallocx_check(void *ptr, size_t size, int flags)
{
    /*
     * Checking for out of range size due to unhandled error in
     * je_mallocx().  Size invalid for the range
     * LLONG_MAX <= size <= ULLONG_MAX
     * which is the result of passing a negative signed number as size
     */
    void *result = NULL;

    if (size >= LLONG_MAX) {
        errno = ENOMEM;
    }
    else {
        result = je_rallocx(ptr, size, flags);
    }
    return result;

}
