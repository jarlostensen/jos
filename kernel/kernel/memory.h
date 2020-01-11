#ifndef JOS_KERNEL_MEMORY_H
#define JOS_KERNEL_MEMORY_H

#include <stdint.h>
#include <stdbool.h>

struct multiboot_info;
void k_mem_init(struct multiboot_info *mboot);

#endif // JOS_KERNEL_MEMORY_H