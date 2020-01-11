
// =========================================================================================
//TODO: should this go into some sort of std library used across different parts of jOS?
#ifdef __GNUC__
#define JOS_PRIVATE_FUNC __attribute__((unused)) static
#else
#define JOS_PRIVATE_FUNC static
#endif
#define JOS_KTRACE(...)
// =========================================================================================

#include "../kernel/kernel/arena_allocator.h"
#include "../kernel/kernel/fixed_allocator.h"
#include "../kernel/kernel/collections.h"

// ========================================================================================
// memory

static vmem_arena_t*   _vmem_arena = 0;
// pools for different sizes of small allocations: 8, 16, 32, 64, 128, 512, 1024, 2048, 4096
static vmem_fixed_t* _small_pools[4];
static size_t _num_small_pools = sizeof(_small_pools)/sizeof(_small_pools[0]);

void* k_mem_alloc(size_t size)
{
    if(!size)
        return 0;
    void* ptr = 0;
    
    if(size>64)
    {
        ptr = vmem_arena_alloc(_vmem_arena, size);        
    }
    else
    {
        size_t pow2 = 3;
        do
        {
            if(size < (1u<<pow2))
            {
                vmem_fixed_t* pool = _small_pools[pow2-3];
                ptr = vmem_fixed_alloc(pool,size);
                break;
            }
            ++pow2;
        }
        while(pow2 < 12);
    }
    if(ptr)
        memset(ptr,0,size);
    
    return ptr;
}

void k_mem_free(void* ptr)
{
    if(!ptr)
        return;
    // for(size_t n = 0; n < _num_small_pools; ++n)
    // {
    //     if(vmem_fixed_in_pool(_small_pools[n],ptr))
    //     {
    //         vmem_fixed_free(_small_pools[n], ptr);
    //         return;
    //     }
    // }    
    vmem_arena_free(_vmem_arena, ptr);
}

// ========================================================================================
// colletions


void test_vector()
{
	vector_t vec;
	vector_create(&vec, 18, sizeof(uintptr_t));
	for(uintptr_t n = 0; n < 43; ++n)
	{
		vector_push_back(&vec, &n);		
	}	

	uintptr_t* data = (uintptr_t*)vec._data;
	for(uintptr_t n = 0; n < 43; ++n)
	{
		if(data[n]!=n)
			break;
	}

	vector_destroy(&vec);
}

int main()
{
	test_vector();
	return 0;
}