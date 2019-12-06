#include "kernel_detail.h"
#include "memory.h"
#include "interrupts.h"
#include "multiboot.h"
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

// in kernel.asm
void _k_move_stack(uintptr_t virt_top);

// in kernel_loader.asm
void _k_page_frame_alloc_ptr(void);
// will map to the above
static page_frame_alloc_t* _page_frame_alloc_ptr = (page_frame_alloc_t*)&_k_page_frame_alloc_ptr;
static size_t _avail_frames = 0;
static uintptr_t _valloc_frame_ptr = (uintptr_t)&_k_virt_end;

// ====================================================================================
// fixed size pool allocator

typedef struct mem_pool_struct
{
    uint8_t     _size_p2;   // power of two unit allocation size
    size_t      _count;
    uint32_t    _free;      // index of first free
} mem_pool_t;

// pools for different sizes of small allocations: 8, 16, 32, 64, 128, 512, 1024, 2048, 4096
static mem_pool_t* _small_pools[9];
static size_t _num_small_pools = sizeof(_small_pools)/sizeof(_small_pools[0]);

static mem_pool_t*     _pool_create(void* mem, size_t size, size_t allocUnitPow2)
{
    if(allocUnitPow2 < 3)
        return 0;
    mem_pool_t* pool = (mem_pool_t*)mem;
    pool->_size_p2 = allocUnitPow2;
    size -= sizeof(mem_pool_t);
    pool->_count = size / (1<<allocUnitPow2);
    pool->_free = 0;
    uint32_t* block = (uint32_t*)((uint8_t*)(pool+1));
    const uint32_t unit_size = 1<<pool->_size_p2;
    for(size_t n = 1; n < pool->_count; ++n)
    {
        *block = (uint32_t)n;
        block += (unit_size >> 2);
    }
    *block = ~0;
    return pool;
}

static void* _pool_alloc(mem_pool_t* pool, size_t size)
{
    if((size_t)(1<<pool->_size_p2) < size || pool->_free == (uint32_t)~0)
    {
        //printf("failed test: %d < %d || 0x%x == 0xffffffff\n", 1<<pool->_size_p2, size, pool->_free);
        return 0;
    }
    const uint32_t unit_size = 1<<pool->_size_p2;
    uint32_t* block = (uint32_t*)((uint8_t*)(pool+1) + pool->_free*unit_size);
    pool->_free = *block; // whatever next it points to
    return block;
}

static void _pool_free(mem_pool_t* pool, void* block)
{
    if(!pool || !block)
        return;
    const uint32_t unit_size = 1<<pool->_size_p2;    
    uint32_t* fblock = (uint32_t*)block;
    *fblock = pool->_free;
    uint32_t* free = (uint32_t*)((uint8_t*)(pool+1) + pool->_free*unit_size);
    *free = (uint32_t)((uintptr_t)fblock - (uintptr_t)(pool+1))/unit_size;
}

static bool _pool_in_pool(mem_pool_t* pool, void* ptr)
{
    return ((uintptr_t)ptr > (uintptr_t)(pool+1)) && ((uintptr_t)ptr < ((uintptr_t)(pool+1) + (1 << pool->_size_p2)));
}

static __attribute__((unused)) void _pool_clear(mem_pool_t* pool)
{
    pool->_free = 0;
    uint32_t* block = (uint32_t*)((uint8_t*)(pool+1));
    const uint32_t unit_size = 1<<pool->_size_p2;
    for(size_t n = 1; n < pool->_count; ++n)
    {
        *block = (uint32_t)n;
        block += (unit_size >> 2);
    }
    *block = ~0;
}

// ====================================================================================

static inline void _flush_tlb_single(uintptr_t addr)
{
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

#define PF_PRESENT          0x1
#define PF_WRITE            0x2
#define PF_USER             0x4
#define PF_RESERVED_WRITE   0x8
#define PF_INSTR_FETCH      0x10

void _k_mem_page_fault_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    JOS_BOCHS_DBGBREAK();
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

void _k_mem_map(uintptr_t virt, uintptr_t phys)
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

void* k_mem_valloc(size_t size, int flags)
{    
    const size_t frames = (size >> 12) + (size & 0xfff) ? 1:0;
    if(!size || _avail_frames<frames)
        return 0;
    uintptr_t ptr = 0;
    switch(flags)
    {
        case kMemValloc_Commit:
        {
            _avail_frames -= frames;
            ptr = _valloc_frame_ptr;
            _valloc_frame_ptr += frames*0x1000;
        }
        break;
        case kMemValloc_Reserve:
        //TODO:
        JOS_ASSERT(false);
        break;
    }
    return (void*)ptr;
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
            const uintptr_t kVirtMapEnd = 0xffc00000;
            
            _avail_frames = avail >> 12;
            // map everything above the page table area to start at the virtual end-of-kernel address
            uintptr_t virt = _valloc_frame_ptr;
            do
            {
                _k_mem_map(virt, phys);
                avail -= 0x1000;
                virt += 0x1000;
                phys += 0x1000;
            } while(avail > 0x1000 && virt < kVirtMapEnd);
            JOS_KTRACE("mem: 0x%x KB in %d frames mapped from [0x%x, 0x%x] -> [0x%x, 0x%x]\n", _avail_frames<<12, _avail_frames, (uintptr_t)&_k_virt_end, (uintptr_t)&_k_phys_end + 0x400000, virt, phys);

            // move our stack to the top of mapped memory where it can grow downwards towards the heap
            virt -= 0x1000;
            _k_move_stack(virt);

            // set up pools            
#define CREATE_SMALL_POOL(i, size, pow2)\
            _small_pools[i] = _pool_create((void*)_valloc_frame_ptr, size, pow2);\
            _valloc_frame_ptr += size;\
            JOS_ASSERT(_valloc_frame_ptr < kVirtMapEnd)

            CREATE_SMALL_POOL(0, 512*8, 3);
            CREATE_SMALL_POOL(1, 512*16, 4);
            CREATE_SMALL_POOL(2, 512*32, 5);
            CREATE_SMALL_POOL(3, 1024*64, 6);
            CREATE_SMALL_POOL(4, 1024*128, 7);
            CREATE_SMALL_POOL(5, 1024*512, 8);
            CREATE_SMALL_POOL(6, 1024*1024, 9);
            CREATE_SMALL_POOL(7, 512*2048, 10);
            CREATE_SMALL_POOL(8, 128*4096, 11);

            return;
        }
    }        

    JOS_KTRACE("error: no available RAM or RAM information\n");
    k_panic();
}

void* k_mem_alloc(size_t size)
{
    if(!size)
        return 0;
    void* ptr = 0;
    size_t pow2 = 3;
    do
    {
        if(size < (1u<<pow2))
        {
            mem_pool_t* pool = _small_pools[pow2-3];
            ptr = _pool_alloc(pool,size);
            break;
        }
        ++pow2;
    }
    while(pow2 < 12);
    return ptr;
}

void k_mem_free(void* ptr)
{
    if(!ptr)
        return;
    for(size_t n = 0; n < _num_small_pools; ++n)
    {
        if(_pool_in_pool(_small_pools[n],ptr))
        {
            _pool_free(_small_pools[n], ptr);
            return;
        }
    }
}