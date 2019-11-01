
#include <stdint.h>
#include "../include/kernel/kernel.h"
#include "interrupts.h"
#include "kernel_detail.h"
#include "paging.h"
#include <stdio.h>
#include <string.h>

// https://wiki.osdev.org/Paging

struct page_directory_struct 
{
    uint8_t     _present : 1;
    uint8_t     _read_write:1;
    uint8_t     _user:1;
    uint8_t     _write_through:1;
    uint8_t     _cache_disable:1;
    uint8_t     _accessed:1;
    uint8_t     _zero_1:1;
    uint8_t     _size:1;
    uint8_t     _zero_2:1;
    uint8_t     _system:3;
    uint32_t    _phys_address:20;
}  __attribute__((packed));
typedef struct page_directory_struct page_directory_t;

struct page_table_struct 
{
    uint8_t     _present : 1;
    uint8_t     _read_write:1;
    uint8_t     _user:1;
    uint8_t     _write_through:1;
    uint8_t     _cached:1;
    uint8_t     _accessed:1;
    uint8_t     _dirty:1;
    uint8_t     _zero:1;
    uint8_t     _global:1;
    uint8_t     _system:3;
    uint32_t    _phys_address:20;
}  __attribute__((packed));
typedef struct page_table_struct page_table_t;

static const size_t kFrameSize = 0x1000;
static const size_t kPageDirEntryRange = 0x400000;

// allocated on init
page_directory_t*    _k_page_dir;

extern void _k_load_page_directory(uint32_t physPageDirStart);
extern void _k_enable_paging(void);

uint32_t k_virt_to_phys(pd_handle_t pd_, uint32_t virt)
{
    if(!pd_)
        return 0;
    page_directory_t* pd = (page_directory_t*)pd_;
    pd = pd + (virt / kPageDirEntryRange);
    if( pd->_present )
    {
        size_t pt_offset = (virt - (virt/kPageDirEntryRange) * kPageDirEntryRange)/kFrameSize;
        page_table_t* pt = (page_table_t*)((uintptr_t)pd->_phys_address << 12) + pt_offset;
        if(!pt->_present)
        {           
            return (pt->_phys_address << 12) + (virt & 0x3fff);    
        }
    }
    return 0;
}

uint32_t _k_alloc_frame(pd_handle_t pd_, uint32_t virt)
{
    if(!pd_)
        return 0;
    page_directory_t* pd = (page_directory_t*)pd_;
    pd = pd + (virt / kPageDirEntryRange);
    if( !pd->_present )
    {
        printf("\n\tpage table not present, creating...");
        page_table_t* pt = (page_table_t*)_k_alloc(sizeof(page_table_t)*1024, kAlign4k);
        memset(pt,0,sizeof(page_table_t)*1024);
        pd->_phys_address = (uint32_t)pt >> 12;
        pd->_present = 1;
        printf("0x%x\n", pd->_phys_address);
    }

    size_t pt_offset = (virt - (virt/kPageDirEntryRange) * kPageDirEntryRange)/kFrameSize;
    page_table_t* pt = (page_table_t*)((uintptr_t)pd->_phys_address << 12) + pt_offset;
    //printf("offset = 0x%x, pt = 0x%x, present = %d, pt->_phys_address = 0x%x ", pt_offset, pt, pt->_present,pt->_phys_address);
    if(!pt->_present)
    {
        printf("\tpage not present, allocating...");
        // allocate a new 4K frame
        void* frame = _k_alloc(0x1000, kAlign4k);
        memset(frame,0,0x1000);
        pt->_phys_address = (uint32_t)frame >> 12;
        pt->_present = 1;
        printf("0x%x\n", pt->_phys_address);
    }
    
    return (pt->_phys_address << 12) + (virt & 0x3fff);    
}

#define PF_PRESENT          0x1
#define PF_WRITE            0x2
#define PF_USER             0x4
#define PF_RESERVED_WRITE   0x8
#define PF_INSTR_FETCH      0x10

void _k_page_fault_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    unsigned long virt;
    asm volatile ( "mov %%cr2, %0" : "=r"(virt) );
    printf("\npage fault @ 0x%x [0x%x:0x%x] error = 0x%x...", virt, cs,eip, error_code);
    uint32_t phys = _k_alloc_frame(_k_page_dir, virt);
    printf("new frame allocated at 0x%x\n", phys);    
}

void k_paging_init()
{    
    _k_page_dir = (page_directory_t*)_k_alloc(sizeof(page_directory_t)*1024, kAlign4k);    
    page_table_t * pt = (page_table_t*)_k_alloc(sizeof(page_table_t)*1024, kAlign4k);
    _k_page_dir->_phys_address = (uint32_t)pt >> 12;
    _k_page_dir->_present = 1;
    printf("\nallocated pd @ 0x%x, first pt @ 0x%x (0x%x)\n", _k_page_dir,_k_page_dir->_phys_address, pt);        
    // identity map the first meg
    for(size_t size = 0; size < 0x800000; size+=0x1000)
    {
        pt->_phys_address = (uint32_t)size >> 12;
        pt->_present = 1;        
        pt++;
    } 
    
    // make it so
    printf("k_paging_init enabling paging, 1st page table @ 0x%x, 0x%x...", _k_page_dir, _k_page_dir->_phys_address);
    k_set_isr_handler(14,_k_page_fault_handler);
    _k_load_page_directory((uint32_t)_k_page_dir);
    _k_enable_paging();  
    printf("ok\n");
    //TEST: page fault
    char* test_pf = (char*)0xf00000;
    *test_pf = 0;
}