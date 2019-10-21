#ifndef JOS_PAGING_H
#define JOS_PAGING_H

typedef void* pd_handle_t;

void k_paging_init();

uint32_t k_virt_to_phys(pd_handle_t pd, uint32_t virt);

#endif // JOS_PAGING_H