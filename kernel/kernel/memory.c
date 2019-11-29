#include "kernel_detail.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>


uintptr_t _k_mem_virt_to_pt_entry(uintptr_t virt)
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
            return page_table[frame_index];
        }
    }
    return 0;
}

uintptr_t k_mem_virt_to_phys(uintptr_t virt)
{
    return _k_mem_virt_to_pt_entry(virt) & ~0xfff;
}

_k_alloc_ptr_t _k_mem_alloc(size_t size)
{
    //TODO:
    (void)size;
    return (_k_alloc_ptr_t){};   
}