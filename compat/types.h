/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#include <stdint.h>

typedef uint32_t u32;
typedef uint64_t u64;

typedef uint32_t __le32;
typedef uint64_t __le64;

static inline __le32 cpu_to_le32(u32 v) {return v;}
static inline __le64 cpu_to_le64(u64 v) {return v;}
static inline u32 le32_to_cpu(__le32 v) {return v;}
static inline u64 le64_to_cpu(__le64 v) {return v;}

static inline void le32_add_cpu(__le32 *n, u32 d) {n += d;}

struct list_head {
	struct list_head *next, *prev;
};

struct hlist_head {
        struct hlist_node *first;
};

struct hlist_node {
        struct hlist_node *next, **pprev;
};

#endif
