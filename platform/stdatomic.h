#ifndef STDATOMIC_H
#define STDATOMIC_H

#include <windows.h>

typedef struct {
    volatile LONG value;
} atomic_int;

#define _Atomic(v) atomic_int

static inline void atomic_store(atomic_int* obj, int desired) {
    InterlockedExchange(&obj->value, (LONG)desired);
}

static inline int atomic_load(atomic_int* obj) {
    return (int)InterlockedCompareExchange(&obj->value, 0, 0);
}

static inline int atomic_fetch_add(atomic_int* obj, int arg) {
    return (int)InterlockedExchangeAdd(&obj->value, (LONG)arg);
}

static inline int atomic_exchange(atomic_int* obj, int desired) {
    return (int)InterlockedExchange(&obj->value, (LONG)desired);
}
#endif // STDATOMIC_H