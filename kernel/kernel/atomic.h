#ifndef JOS_ATOMIC_H
#define JOS_ATOMIC_H

#include <stdint.h>

typedef struct atomic_int_struct 
{
    volatile int _val;
} atomic_int_t;

typedef struct atomic_int32_struct 
{
    volatile int32_t _val;
} atomic_int32_t;

typedef struct atomic_uint32_struct 
{
    volatile uint32_t _val;
} atomic_uint32_t;

typedef struct atomic_int64_struct
{
    volatile int64_t _val __attribute__ ((aligned (8)));
} atomic_int64_t;

#endif // JOS_ATOMIC_H