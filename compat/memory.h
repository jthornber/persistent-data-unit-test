#ifndef compat_memory_h_INCLUDED
#define compat_memory_h_INCLUDED

#include <stdlib.h>
#include <string.h>

#define MAX_ERRNO       4095
#define IS_ERR_VALUE(x) ((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

static inline void * ERR_PTR(long error)
{
	        return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
	        return (long) ptr;
}

static inline bool IS_ERR(const void *ptr)
{
	        return IS_ERR_VALUE((unsigned long)ptr);
}

enum {
	GFP_NOIO,
	GFP_NOFS,
	GFP_KERNEL
};

static inline void *kmalloc(size_t s, int _) {return malloc(s);}
static inline void kfree(void *ptr) {free(ptr);}

static inline void *zalloc(size_t len) {
	void *ptr = malloc(len);
	if (ptr)
		memset(ptr, 0, len);
	return ptr;
}

#endif

