
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
page_directory_t    _k_page_dir[1024] __attribute__ ((aligned (4096)));
//TESTING
page_table_t        _1st_pt[1024] __attribute__ ((aligned (4096)));

extern void _k_load_page_directory(uint32_t physPageDirStart);
extern void _k_enable_paging(void);

uint32_t k_virt_to_phys(pd_handle_t pd_, uint32_t virt)
{
    if(!pd_)
        return 0;
    page_directory_t* pd = (page_directory_t*)pd_;
    pd = pd + (virt / kPageDirEntryRange);
    printf("\tpd = 0x%x ", pd);
    if( pd->_present )
    {
        size_t pt_offset = (virt - (virt/kPageDirEntryRange) * kPageDirEntryRange)/kFrameSize;
        page_table_t* pt = (page_table_t*)((uintptr_t)pd->_phys_address << 12) + pt_offset;
        printf("offset = 0x%x, pt = 0x%x, present = %d, pt->_phys_address = 0x%x ", pt_offset, pt, pt->_present,pt->_phys_address);
        if(pt->_present)
        {
            return (pt->_phys_address << 12) + (virt & 0x3fff);
        }
        else
        {
            printf("page table is wrong!\n");
        }
    }
    else
    {
        printf("page dir is wrong!\n");
    }
    
    //or else...what?
    return 0;
}

void k_paging_init()
{
    // identity map the first meg
    //_k_page_dir = (page_directory_t*)_k_alloc(sizeof(page_directory_t)*1024, k4k);    
    //page_table_t * pt = (page_table_t*)_k_alloc(sizeof(page_table_t)*1024, k4k);
    page_table_t* pt = _1st_pt;
    _k_page_dir->_phys_address = (uint32_t)pt >> 12;
    _k_page_dir->_present = 1;
    printf("\nallocated pd @ 0x%x, first pt @ 0x%x (0x%x)\n", _k_page_dir,_k_page_dir->_phys_address, pt);        
    for(size_t size = 0; size < 0x200000; size+=0x1000)
    {
        pt->_phys_address = (uint32_t)size >> 12;
        pt->_present = 1;        
        pt++;
    } 
    
    // make it so
    printf("k_paging_init enabling paging, 1st page table @ 0x%x, 0x%x...", _k_page_dir, _k_page_dir->_phys_address);
    _k_load_page_directory((uint32_t)_k_page_dir);
    _k_enable_paging();  
    //uint32_t map = k_virt_to_phys(_k_page_dir,0x10173b);
    // /printf("paging mapped 0x%x to 0x%x\n", 0x10173b, map);
    printf("ok\n");
    
}