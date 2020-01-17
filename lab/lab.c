
// =========================================================================================

#include <kernel/kernel.h>
#include <kernel/atomic.h>
#include <arena_allocator.h>
#include <fixed_allocator.h>
#include <collections.h>

// ========================================================================================
// memory

static vmem_arena_t* _vmem_arena = 0;
// pools for different sizes of small allocations: 8, 16, 32, 64 bytes
static vmem_fixed_t* _small_pools[4];
static size_t _num_small_pools = sizeof(_small_pools) / sizeof(_small_pools[0]);

void* k_mem_alloc(size_t size)
{
	if (!size)
		return 0;

	if (size > 64)
	{
		return vmem_arena_alloc(_vmem_arena, size);
	}
	size_t pow2 = 3;
	do
	{
		if (size <= (1u << pow2))
		{
			vmem_fixed_t* pool = _small_pools[pow2 - 3];
			return vmem_fixed_alloc(pool, size);
		}
		++pow2;
	} while (pow2 < 7);

	return 0;
}

void k_mem_free(void* ptr)
{
	if (!ptr)
		return;
	for (size_t n = 0; n < _num_small_pools; ++n)
	{
		if (vmem_fixed_in_pool(_small_pools[n], ptr))
		{
			vmem_fixed_free(_small_pools[n], ptr);
			return;
		}
	}
	vmem_arena_free(_vmem_arena, ptr);
}

void test_mem()
{
	// create a master heap using the general purpose arena allocator
	size_t arena_size = 0x100000;
	void* memory = malloc(arena_size);

	_vmem_arena = vmem_arena_create(memory, arena_size);
	JOS_KTRACE("mem: %d KB in %d frames allocated for pools, starts at 0x%x, ends at 0x%x\n", arena_size / 1024, arena_size >> 12, (uintptr_t)&_k_virt_end, (uintptr_t)virt);

	// now allocate some fixed-size pools for small-ish allocations out of the main heap
#define CREATE_SMALL_POOL(i, size, pow2)\
    _small_pools[i] = vmem_fixed_create(vmem_arena_alloc(_vmem_arena,size), size, pow2)

	CREATE_SMALL_POOL(0, 512 * 8, 3);
	CREATE_SMALL_POOL(1, 512 * 16, 4);
	CREATE_SMALL_POOL(2, 512 * 32, 5);
	CREATE_SMALL_POOL(3, 512 * 64, 6);

	for (int n = 8; n < 0x1000; n += 8)
	{
		volatile void* ptr = k_mem_alloc(n);
		memset(ptr, 0xff, n);
		k_mem_free(ptr);
	}

	free(memory);
}

// ========================================================================================
// colletions


void test_vector()
{
	vector_t vec;
	vector_create(&vec, 18, sizeof(uintptr_t));
	for (uintptr_t n = 0; n < 43; ++n)
	{
		vector_push_back(&vec, &n);
	}

	uintptr_t* data = (uintptr_t*)vec._data;
	for (uintptr_t n = 0; n < 43; ++n)
	{
		if (data[n] != n)
			break;
	}

	vector_destroy(&vec);
}

// ========================================================================================
// misc

void test_atomic()
{
	atomic_int32_t aint32;
	atomic_store(&aint32,42);
	int32_t val = atomic_load(&aint32);
	if(val!=42)
		return;
}

int main()
{
	test_vector();
	test_mem();
	return 0;
}