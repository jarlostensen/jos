
// =========================================================================================

#include <kernel/kernel.h>
#include <kernel/atomic.h>
#include <arena_allocator.h>
#include <fixed_allocator.h>
#include <collections.h>

#include <assert.h>

#include "../libc/libc_internal.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <intrin.h>

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
	_JOS_KTRACE("mem: %d KB in %d frames allocated for pools, starts at 0x%x, ends at 0x%x\n", arena_size / 1024, arena_size >> 12, (uintptr_t)&_k_virt_end, (uintptr_t)virt);

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


void test_vector(void)
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
		assert(data[n] == n);
	}

	int e = 101;
	vector_set_at(&vec, 5, &e);
	memcpy(&e, vector_at(&vec, 5), sizeof(e));

	vector_destroy(&vec);
}

void test_queue(void)
{
	queue_t	queue;
	queue_create(&queue, 21, sizeof(uintptr_t));
	assert(queue_is_empty(&queue));

	for(uintptr_t n = 0; n < 21; ++n)
	{
		queue_push(&queue, &n);		
		uintptr_t f = *((uintptr_t*)queue_front(&queue));
		assert(f==0);
	}
	assert(queue_is_full(&queue));

	for(uintptr_t n = 0; n < 100; ++n)
	{
		queue_pop(&queue);
		queue_push(&queue,&n);
		assert(queue_is_full(&queue));
	}

	queue_destroy(&queue);

	queue_t queues[4];
	queue_create(queues+0,8,sizeof(int));
	queue_create(queues+1,8,sizeof(int));
	queue_create(queues+2,8,sizeof(int));
	queue_create(queues+3,8,sizeof(int));
	queue_t* q1 = queues + 1;
	queue_t* q2 = queues + 2;

	int n = 42;
	queue_push(q1, &n);
	n = *((int*)queue_front(q1));

	queue_create(&queue, 8, sizeof(char*));
	const char* str1 = "foo";
	queue_push(&queue, &str1);
	const char* f = *((char**)queue_front(&queue));
	assert(f==str1);
}

// ========================================================================================
// misc

extern int _JOS_LIBC_FUNC_NAME(printf)(const char* __restrict format, ...);

void test_stdio(void)
{
	char garbage[4] = {0};
	garbage[0] = 0xf3;
	garbage[1] = 0x01;
	garbage[3] = 0xcc;
	_JOS_LIBC_FUNC_NAME(printf)("hello world %s!\n", garbage);
}


extern void _k_trace(const char* __restrict channel, const char* __restrict format,...);

#undef _JOS_KTRACE
#define _JOS_KTRACE(channel, msg,...) _k_trace(channel, msg,##__VA_ARGS__)
static const char* kTestChannel = "test";

void test_ktrace(void)
{	
	_JOS_KTRACE(kTestChannel,"b b boooo?\n");
	uint8_t _bus_id = 1;
	uint8_t bus_id[6];
	sprintf_s(bus_id, 6, "%s", "ISA");
	_JOS_KTRACE(kTestChannel,"bus %d, %s\n", _bus_id, bus_id);
	char garbage[4] = {0};
	garbage[0] = 0xf3;
	garbage[1] = 0x01;
	garbage[3] = 0xcc;
	_k_trace(kTestChannel,"hello %s!\n", garbage);
}

void test_atomic(void)
{
	
}

void test_hypervisor(void)
{
     // check for hypervisor(s)
    int regs[4];
    __cpuid(regs, 1);
    const auto isVMM = ((regs[2] >> 31) & 1) == 1;
    if(isVMM)
    {
        // we're in a hypervisor, is it one we know about?
        char hyper_vendor_id[13] = { 0 };
        __cpuid(regs, 0x40000000);
        memcpy(hyper_vendor_id, regs + 1, 3 * sizeof(int));
		_JOS_KTRACE(kTestChannel, "running in hypervisor mode, vendor id is \"%s\"\n", hyper_vendor_id);
    }
}

// ========================================================================================
// tty console

extern void vga_init(void);
extern void vga_draw(void);
#include <kernel/output_console.h>

void init_console(void)
{    
	output_console_init();
}

void test_console(void)
{	
	for(int n = 0; n < 50; ++n)
	{
		_JOS_LIBC_FUNC_NAME(printf)("line\t%d\n", n);
		vga_draw();
	}
	_JOS_LIBC_FUNC_NAME(printf)("last line");	
}

int main(void)
{
	init_console();
	//test_console();

	test_hypervisor();
	test_vector();
	test_mem();
	test_queue();
	test_stdio();
	test_ktrace();
	
 	output_console_destroy(&_stdout);
	return 0;
}