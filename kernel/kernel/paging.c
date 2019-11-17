
#include "../include/kernel/kernel.h"
#include "interrupts.h"
#include "kernel_detail.h"
#include "paging.h"
#include "cpu.h"
#include <stdio.h>
#include <string.h>

// https://wiki.osdev.org/Paging
// NOTE: these are really just different views on the same 32(64)-bit data.

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

// ========================================= 
// long mode page structures
// see for example https://en.wikipedia.org/wiki/X86-64#Virtual_address_space_details
// and Intel IA Dev Guide Vol. 3A 4-19
// 
// we have four levels of page tables:
// * Page-Map Level-4 Table (PML4)
// * Page-Directory Pointer Table (PDP)
// * Page-Directory Table (PD)
// * Page Table (PT).
// 
//NOTE: never set in PT or PML4, 1 Gig pages if set in PDP, 2MB if set in PD
    
// Intel IA Dev Guide Vol. 3A Table 4-14
struct pml4_struct 
{
    uint8_t     _present : 1;
    uint8_t     _read_write:1;
    uint8_t     _user:1;
    uint8_t     _write_through:1;
    uint8_t     _cache_disable:1;
    uint8_t     _accessed:1;
    uint8_t     _dirty:1;
    uint8_t     _huge:1;
    uint8_t     _global:1;
    uint8_t     _os_1:3;
    uint64_t    _phys_address:40;
    uint16_t    _os_2:11;
    uint8_t     _no_execute:1;
}  __attribute__((packed));
typedef struct pml4_struct pml4_t;

static const size_t kFrameSize = 0x1000;
static const size_t kPageDirEntryRange = 0x400000;

// allocated on init
page_directory_t*       _k_page_dir;
pml4_t*                 _k_page_l4_dir;

// counters
static size_t _page_tables_allocated = 0;
static size_t _page_frames_allocated = 0;
static bool _1gig_pages_supported = false;
static bool _4mb_pages_supported = false;

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

page_table_t* _alloc_page_table()
{
    page_table_t* pt = (page_table_t*)k_alloc(sizeof(page_table_t)*1024, kAlign4k);
    memset(pt,0,sizeof(page_table_t)*1024);
    return pt;
}

// allocate a frame for virt, if needed
// returns physical address in new frame
uint32_t _get_frame(pd_handle_t pd_, uint32_t virt)
{
    if(!pd_)
        return 0;
    page_directory_t* pd = (page_directory_t*)pd_;
    pd = pd + (virt / kPageDirEntryRange);
    if( !pd->_present )
    {
        //printf("\n\tpage table not present, creating...");
        page_table_t* pt = _alloc_page_table();
        pd->_phys_address = (uint32_t)pt >> 12;
        pd->_present = 1;
        //printf("0x%x\n", pd->_phys_address);
        ++_page_tables_allocated;
    }

    size_t pt_offset = (virt - (virt/kPageDirEntryRange) * kPageDirEntryRange)/kFrameSize;
    page_table_t* pt = (page_table_t*)((uintptr_t)pd->_phys_address << 12) + pt_offset;
    //printf("offset = 0x%x, pt = 0x%x, present = %d, pt->_phys_address = 0x%x ", pt_offset, pt, pt->_present,pt->_phys_address);
    if(!pt->_present)
    {
        //printf("\tpage frame not present, allocating...");
        // allocate a new 4K frame
        void* frame = k_alloc(0x1000, kAlign4k);
        memset(frame,0,0x1000);
        pt->_phys_address = (uint32_t)frame >> 12;
        pt->_present = 1;
        //printf("0x%x\n", pt->_phys_address);
        ++_page_frames_allocated;
    }
    
    return (pt->_phys_address << 12) + (virt & 0x3fff);    
}

#define PF_PRESENT          0x1
#define PF_WRITE            0x2
#define PF_USER             0x4
#define PF_RESERVED_WRITE   0x8
#define PF_INSTR_FETCH      0x10

static void _page_fault_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    unsigned long virt;
    asm volatile ( "mov %%cr2, %0" : "=r"(virt) );
    printf("\npage fault @ 0x%x [0x%x:0x%x] error = 0x%x...", virt, cs,eip, error_code);
    uint32_t phys = _get_frame(_k_page_dir, virt);
    printf("new frame allocated at 0x%x\n", phys);    
}

//WIP:
__attribute__((unused)) static void _init_l4_paging() 
{
    // l4
    _k_page_l4_dir = (pml4_t*)k_alloc(sizeof(pml4_t)*512, kAlign4k);
    memset(_k_page_l4_dir,0,sizeof(pml4_t)*512);
    pml4_t* pt = (pml4_t*)k_alloc(sizeof(pml4_t)*512, kAlign4k);
    memset(pt, 0, sizeof(pml4_t)*512);
    _k_page_l4_dir->_phys_address = (uint64_t)((uint32_t)pt >> 12);
    _k_page_l4_dir->_present = 1;
    _k_page_l4_dir->_read_write = 1;

    // l3
    pml4_t* ptn = (pml4_t*)k_alloc(sizeof(pml4_t)*512, kAlign4k);
    memset(ptn, 0, sizeof(pml4_t)*512);
    pt->_phys_address = (uint64_t)((uint32_t)ptn >> 12);
    pt->_present = 1;
    pt->_read_write = 1;

    // l2
    if(_1gig_pages_supported)
    {
        // this selects a one gig page size
        pt->_huge = 1;
        
        // we just need one page to cover the whole thing from 0 to 1Gig
        ptn->_present = 1;
        ptn->_read_write = 1;        
        ptn->_phys_address = 0;
    }
    else
    {
        // use 2MB pages (NOTE: 4MB pages are only used in non-PAE modes, i.e. in 32-bit and not long mode...?)
        //ZZZ: just mapping 4megs in total here
        uint64_t addr = 0x200000;
        for(int p = 0; p < 2; ++p)
        {
            // select 2MB page
            ptn->_huge = 1;
            ptn->_present = 1;
            ptn->_read_write = 1;
            ptn->_phys_address = addr;                        
            addr+=0x200000;
        }
    }

    //TODO: enable
}

static void _init_32bit_paging()
{
    _k_page_dir = (page_directory_t*)k_alloc(sizeof(page_directory_t)*1024, kAlign4k);    
    memset(_k_page_dir, 0, sizeof(sizeof(page_directory_t)*1024));
    page_table_t* pt = _alloc_page_table();
    _k_page_dir->_phys_address = (uint32_t)pt >> 12;
    _k_page_dir->_present = 1;
    _page_tables_allocated = 1;

    // identity map the first 4 megs
    for(size_t size = 0; size < 0x400000; size+=0x1000)
    {
        pt->_phys_address = (uint32_t)size >> 12;
        pt->_present = 1;
        ++pt;
        // not strictly "allocated" but at least assigned...
        ++_page_frames_allocated;
    } 
    // make it so
    JOS_KTRACE("%d page tables and %d frames available for %d bytes of physical memory\n", _page_tables_allocated, _page_frames_allocated, _page_frames_allocated*0x1000);
    k_set_isr_handler(14,_page_fault_handler);
    _k_load_page_directory((uint32_t)_k_page_dir);    
    _k_enable_paging();
}

void k_paging_init()
{   
    JOS_KTRACE("k_paging_init\n");

    unsigned int _eax = 0, _ebx, _ecx, _edx = 0;
    __get_cpuid(0x1, &_eax, &_ebx, &_ecx, &_edx);
    if( (_edx & (1<<3)) == (1<<3))
    {
        JOS_KTRACE("4MB pages supported\n");
        _4mb_pages_supported = true;
    }
    
    __get_cpuid(0x80000001, &_eax, &_ebx, &_ecx, &_edx);
    if( (_edx & (1<<26)) == (1<<26))
    {
        JOS_KTRACE("1Gig pages supported\n");
        _1gig_pages_supported = true;
    }
    
    
    _init_32bit_paging();
}