#ifndef _JOS_KERNEL_MEMORY_H
#define _JOS_KERNEL_MEMORY_H

#include <stdint.h>
#include <stdbool.h>

struct multiboot_info;
void k_mem_init(struct multiboot_info *mboot);

#endif // _JOS_KERNEL_MEMORY_H