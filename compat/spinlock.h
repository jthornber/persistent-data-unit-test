#ifndef compat_spinlock_h_INCLUDED
#define compat_spinlock_h_INCLUDED

struct spinlock {
};

typedef struct spinlock spinlock_t;

static inline void spin_lock_init(spinlock_t *s) {}
static inline void spin_lock(spinlock_t *s) {}
static inline void spin_unlock(spinlock_t *s) {}

#endif

