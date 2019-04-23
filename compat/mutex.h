#ifndef compat_mutex_h_INCLUDED
#define compat_mutex_h_INCLUDED

struct mutex {
};

static inline void mutex_init(struct mutex *m) {}
static inline void mutex_lock(struct mutex *m) {}
static inline void mutex_unlock(struct mutex *m) {}

#endif

