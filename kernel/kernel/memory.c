#include "../include/kernel/kernel.h"
#include "interrupts.h"
#include "kernel_detail.h"
#include "cpu.h"
#include <stdio.h>
#include <string.h>

uintptr_t k_mem_virt_to_phys(uintptr_t virt)
{
    // we're using the magic "recursive page directory entry" @ 0xfffff000

    const uintptr_t* page_dir = (const uintptr_t*)0xfffff000;
    const uintptr_t page_table_index = virt >> 22;
    
    // we can use this up as a virtual address because of the "self" mapping
    if(page_dir[page_table_index] & 1)
    {
        const uintptr_t frame_index = (virt & 0x3fffff) >> 12;
        const uintptr_t* page_table = (const uintptr_t*)(0xffc00000 | (page_table_index<<12));
        if(page_table[frame_index] & 1)
        {
            return page_table[frame_index] & ~0xfff;
        }
    }
    return 0;
}
