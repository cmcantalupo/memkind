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

#ifndef memkind_hbw_include_h
#define memkind_hbw_include_h
#ifdef __cplusplus
extern "C" {
#endif

#include "memkind.h"

static const char * const MEMKIND_BANDWIDTH_PATH = "/etc/memkind/node-bandwidth";

int memkind_hbw_check_available(struct memkind *kind);
int memkind_hbw_get_mbind_nodemask(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
void memkind_hbw_init_once(void);
void memkind_hbw_hugetlb_init_once(void);
void memkind_hbw_preferred_init_once(void);
void memkind_hbw_preferred_hugetlb_init_once(void);

extern const struct memkind_ops MEMKIND_HBW_OPS;
extern const struct memkind_ops MEMKIND_HBW_HUGETLB_OPS;
extern const struct memkind_ops MEMKIND_HBW_PREFERRED_OPS;
extern const struct memkind_ops MEMKIND_HBW_PREFERRED_HUGETLB_OPS;


#ifdef __cplusplus
}
#endif
#endif
