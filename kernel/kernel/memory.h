#ifndef JOS_MEMORY_H
#define JOS_MEMORY_H

#include <stdint.h>
#include <stdbool.h>

struct multiboot_info;
void k_mem_init(struct multiboot_info *mboot);

// return physical address or 0 (which is of course valid if virt is 0...)
uintptr_t k_mem_virt_to_phys(uintptr_t virt);

bool TEST_write_protect(uintptr_t virt);

// return the page table entry for the frame containing virt
uintptr_t _k_mem_virt_to_pt_entry(uintptr_t virt);

void k_mem_map(uintptr_t virt, uintptr_t phys);

#endif // JOS_MEMORY_H