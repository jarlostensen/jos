#ifndef JOS_KERNEL_DETAIL_H
#define JOS_KERNEL_DETAIL_H

#include <stdint.h>
#include <stddef.h>

void _k_enable_interrupts();
void _k_disable_interrupts();
void _k_halt_cpu();

typedef enum alignment_enum
{
    kNone = 0,
    k16 = 2,
    k32 = 4,
    k64 = 8,
    k128 = 16,
    k256 = 32,
    k512 = 64,
    k4k = 0x1000,
} alignment_t;
void* _k_alloc(size_t bytes, alignment_t alignment);
void _k_alloc_init();

#endif // JOS_KERNEL_DETAIL_H