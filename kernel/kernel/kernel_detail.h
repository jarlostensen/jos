#ifndef JOS_KERNEL_DETAIL_H
#define JOS_KERNEL_DETAIL_H

#include <stdint.h>
#include <stddef.h>


extern void enable_interrupts();
extern void disable_interrupts();
extern void halt_cpu();

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
void* kalloc(size_t bytes, alignment_t alignment);
void kalloc_init();

void k_panic();

#endif // JOS_KERNEL_DETAIL_H