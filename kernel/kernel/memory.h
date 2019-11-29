#ifndef JOS_MEMORY_H
#define JOS_MEMORY_H

#include <stdint.h>

// return physical address or 0 (which is of course valid if virt is 0...)
uintptr_t k_mem_virt_to_phys(uintptr_t virt);

// return the page table entry for the frame containing virt
uintptr_t _k_mem_virt_to_pt_entry(uintptr_t virt);

struct _k_alloc_ptr_struct {
    uintptr_t       _virt;
    uintptr_t       _phys;
}   __attribute__((packed));
typedef struct _k_alloc_ptr_struct _k_alloc_ptr_t;
// allocate size bytes and return the virtual and physical address structure
_k_alloc_ptr_t _k_mem_alloc(size_t size);

#endif // JOS_MEMORY_H