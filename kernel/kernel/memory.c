#include "kernel_detail.h"
#include "memory.h"
#include "interrupts.h"
#include "multiboot.h"
#include <stdio.h>
#include <string.h>

// in link_high.ld
void _k_phys_end(void);
void _k_phys_start(void);
void _k_virt_end(void);
void _k_virt_start(void);

typedef struct page_frame_alloc_struct
{
    uintptr_t   _phys;
    uintptr_t   _virt;
} __attribute__((packed))
page_frame_alloc_t;

// in kernel_loader.asm
void _k_page_frame_alloc_ptr(void);
// will map to the above
static page_frame_alloc_t* _page_frame_alloc_ptr = (page_frame_alloc_t*)&_k_page_frame_alloc_ptr;
static size_t _avail_frames = 0;

static inline void _flush_tlb_single(uintptr_t addr)
{
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

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


#define PF_PRESENT          0x1
#define PF_WRITE            0x2
#define PF_USER             0x4
#define PF_RESERVED_WRITE   0x8
#define PF_INSTR_FETCH      0x10

void _k_mem_page_fault_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    unsigned long virt;
    asm volatile ( "mov %%cr2, %0" : "=r"(virt) );
    printf("\npage fault @ 0x%x [0x%x:0x%x] error = 0x%x...", virt, cs,eip, error_code);    
}

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

static uintptr_t _mem_alloc_frames(size_t frameCount)
{
    if(_avail_frames < frameCount)
        return 0;
    uintptr_t phys = _page_frame_alloc_ptr->_phys;
    _page_frame_alloc_ptr->_phys += 0x1000 * frameCount;
    _page_frame_alloc_ptr->_virt += 0x1000 * frameCount;
    _avail_frames -= frameCount;
    // always 4K aligned, low 12 bits 0
    return phys;
}

void k_mem_map(uintptr_t virt, uintptr_t phys)
{
    virt &= ~0xfff;
    phys &= ~0xfff;

    uintptr_t* page_dir = (uintptr_t*)0xfffff000;
    const uintptr_t page_table_index = virt >> 22;
    
    // we can use this up as a virtual address because of the "self" mapping
    if((page_dir[page_table_index] & 1)==0)
    {
        // page table doesn't exist, allocate it and mark it as present
        uintptr_t pt_frame = _mem_alloc_frames(1);
        page_dir[page_table_index] = pt_frame | 1;
        // flush TLB to make sure it gets reloaded when we next access
        _flush_tlb_single(pt_frame);
        // clear the table itself
        memset((void*)(0xffc00000 | (page_table_index<<12)), 0, 0x1000);
    }

    const uintptr_t frame_index = (virt & 0x3fffff) >> 12;
    uintptr_t* page_table = (uintptr_t*)(0xffc00000 | (page_table_index<<12));
    if((page_table[frame_index] & 1)==0)
    {
        page_table[frame_index] = phys | 1;
        _flush_tlb_single(phys);
    }
    //TODO: error? or do we allow re-mapping?
}

void k_mem_init(struct multiboot_info *mboot)
{
    // end of kernel + 4Megs to skip past area used by page tables
    uintptr_t phys = (uintptr_t)&_k_phys_end + 0x400000;    
    if(mboot->flags & MULTIBOOT_INFO_MEMORY)
    {
        JOS_KTRACE("mem: mem_lower = %d KB, mem_upper = %d KB\n", mboot->mem_lower, mboot->mem_upper);

        // available RAM above phys (within a single region)
        size_t avail = 0;
        for (multiboot_memory_map_t* mmap = (multiboot_memory_map_t *) mboot->mmap_addr; 
            (unsigned long) mmap <mboot->mmap_addr + mboot->mmap_length; 
            mmap = (multiboot_memory_map_t *) ((unsigned long) mmap + mmap->size + sizeof (mmap->size)))
        {
            if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
            {                
                JOS_KTRACE("mem: available 0x%x bytes @ 0x%x\n",
                            (unsigned) mmap->len,
                            (unsigned) mmap->addr);
                if( phys > mmap->addr && phys < mmap->addr+mmap->len)
                {
                    avail = (size_t)(mmap->addr+mmap->len) - (size_t)phys;                    
                }
            }
        }
        if(avail > 0x1000)
        {
            _avail_frames = avail >> 12;
            // map everything above the page table area to start at the virtual end-of-kernel address
            uintptr_t virt = (uintptr_t)&_k_virt_end;
            do
            {
                k_mem_map(virt, phys);
                avail -= 0x1000;
                virt += 0x1000;
                phys += 0x1000;
            } while(avail > 0x1000 && virt < 0xffc00000);
            JOS_KTRACE("mem: 0x%x KB in %d frames mapped from [0x%x, 0x%x] -> [0x%x, 0x%x]\n", _avail_frames<<12, _avail_frames, (uintptr_t)&_k_virt_end, (uintptr_t)&_k_phys_end + 0x400000, virt, phys);
            return;
        }
    }        

    JOS_KTRACE("error: no available RAM or RAM information\n");
    k_panic();
}