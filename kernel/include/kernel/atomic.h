#ifndef JOS_ATOMIC_H
#define JOS_ATOMIC_H

#include <stdint.h>
#include <jos.h>

typedef struct _atomic_int
{
    volatile JOS_ALIGN(int,_val,4);
} atomic_int_t;

typedef struct _atomic_int32
{
    volatile JOS_ALIGN(int64_t,_val, 4);
} atomic_int32_t;

typedef struct _atomic_uint32
{
    volatile JOS_ALIGN(uint32_t,_val, 4);
} atomic_uint32_t;

typedef struct _atomic_int64
{
    volatile JOS_ALIGN(int64_t,_val,8);
} atomic_int64_t;

typedef struct _atomic_uint64
{
    volatile JOS_ALIGN(uint64_t,_val,8);
} atomic_uint64_t;

#define atomic_load(atomic) ((atomic)->_val)
#define atomic_store(atomic,val) (((atomic)->_val) = (val))

#ifdef __GNUC__
static inline int32_t test_it(atomic_int32_t* atomic)
{
	return __atomic_load_n(&atomic->_val, __ATOMIC_RELAXED);
}
#endif 

#endif // JOS_ATOMIC_H