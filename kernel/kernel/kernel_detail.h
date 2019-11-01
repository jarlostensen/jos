#ifndef JOS_KERNEL_DETAIL_H
#define JOS_KERNEL_DETAIL_H

#include <stdint.h>
#include <stddef.h>

void _k_enable_interrupts();
void _k_disable_interrupts();
void _k_halt_cpu();
// switch to the given page directory (i.e. load cr3)
void _k_load_page_directory(uint32_t physPageDirStart);

typedef enum alignment_enum
{
    kNone = 0,
    kAlign16 = 2,
    kAlign32 = 4,
    kAlign64 = 8,
    kAlign128 = 16,
    kAlign256 = 32,
    kAlign512 = 64,
    kAlign4k = 0x1000,
} alignment_t;
void* _k_alloc(size_t bytes, alignment_t alignment);
void _k_alloc_init();

//TODO: read current NMI status + CMOS management 
// non-maskable int enable
// inline void _k_enable_nmi() {
//     k_outb(0x70, k_inb(0x70) & 0x7F);
//  }
 
//  // non-maskable int disable
//  inline void _k_disable_nmi() {
//     k_outb(0x70, k_inb(0x70) | 0x80);
//  }

#endif // JOS_KERNEL_DETAIL_H