#ifndef COMPAT_BUG_H
#define COMPAT_BUG_H

#include <assert.h>

#define BUG_ON(x...) assert(# x)

#endif

