#ifndef compat_device_mapper_h_INCLUDED
#define compat_device_mapper_h_INCLUDED

#include <stdarg.h>
#include <stdio.h>

static inline void DMERR(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

#define DMERR_LIMIT(args ...) DMERR(#args)

typedef uint64_t sector_t;

static inline sector_t dm_sector_div_up(sector_t n, sector_t d)
{
	return (n + (d - 1)) / d;
}

static inline uint64_t do_div(uint64_t n, uint64_t d)
{
	return n / d;
}

#define __cmp(x, y, op) ((x) op (y) ? (x) : (y))

#define max(x, y)       __cmp(x, y, >)
#define max_t(type, x, y)       __cmp((type)(x), (type)(y), >)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif

