#ifndef JOS_MEMORY_H
#define JOS_MEMORY_H

#include <stdint.h>

// return physical address or 0 (which is of course valid if virt is 0...)
uintptr_t k_mem_virt_to_phys(uintptr_t virt);

#endif // JOS_MEMORY_H