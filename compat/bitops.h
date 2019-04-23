#ifndef compat_bitops_h_INCLUDED
#define compat_bitops_h_INCLUDED

#include <stdbool.h>

static inline bool test_bit_le(unsigned bit, void *mem) {return true;}
static inline void __set_bit_le(unsigned bit, void *mem) {}
static inline void __clear_bit_le(unsigned bit, void *mem) {}

#endif

