
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
	for (uintptr_t n = 0; n < vector_size(&vec); ++n)
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

	for (uintptr_t n = 0; n < 21; ++n)
	{
		queue_push(&queue, &n);
		uintptr_t f = *((uintptr_t*)queue_front(&queue));
		assert(f == 0);
	}
	assert(queue_is_full(&queue));

	for (uintptr_t n = 0; n < 100; ++n)
	{
		queue_pop(&queue);
		queue_push(&queue, &n);
		assert(queue_is_full(&queue));
	}

	queue_destroy(&queue);

	queue_t queues[4];
	queue_create(queues + 0, 8, sizeof(int));
	queue_create(queues + 1, 8, sizeof(int));
	queue_create(queues + 2, 8, sizeof(int));
	queue_create(queues + 3, 8, sizeof(int));
	queue_t* q1 = queues + 1;
	queue_t* q2 = queues + 2;

	int n = 42;
	queue_push(q1, &n);
	n = *((int*)queue_front(q1));

	queue_create(&queue, 8, sizeof(char*));
	const char* str1 = "foo";
	queue_push_ptr(&queue, str1);
	const char* f = (char*)queue_front_ptr(&queue);
	assert(f == str1);
}

typedef struct _vmem_range vmem_range_t;

struct _vmem_range
{
	uintptr_t	_start;
	size_t		_pages;
};

// free ranges, i.e. ranges that are available, live in this hash table
// we don't expect a lot of free ranges, so this table doesn't have to be big...
// this is the only table that we have to do any searches in.
#define kTableSize 17
static vector_t _table[kTableSize];

// when we allocate a range and remove it from the range table we allocate the handle for it from here, may need to grow.
static char _range_pool_buffer[8 * 1024];
static vmem_fixed_t* _range_pool = 0;

static uint32_t* _memory_map = 0;
static size_t _memory_map_dword_count = 0;

#define _JOS_KERNEL_PAGE_SIZE 0x1000
#define _JOS_KERNEL_PAGE_MOD 0xfff


#define _JOS_KERNEL_VMEM_ADDRESS_POOL_SIZE 128
static uintptr_t	_vmem_address_pool[_JOS_KERNEL_VMEM_ADDRESS_POOL_SIZE];
static size_t		_vmem_range_info[_JOS_KERNEL_VMEM_ADDRESS_POOL_SIZE];

void vmem_init(void)
{
	// each bit denotes a 4K page
	size_t memory_map_byte_size = (0xffffffff / _JOS_KERNEL_PAGE_SIZE) / 8;
	_memory_map = malloc(memory_map_byte_size);
	// nothing allocated
	memset(_memory_map, 0, memory_map_byte_size);
	_memory_map_dword_count = memory_map_byte_size / sizeof(uint32_t);
	_range_pool = vmem_fixed_create(_range_pool_buffer, 1024, 3);
}

static void _vmem_mark_range(uintptr_t start, size_t pages)
{
	_JOS_ASSERT((start & _JOS_KERNEL_PAGE_MOD) == 0);
	size_t start_page = (start / _JOS_KERNEL_PAGE_SIZE);
	size_t dword_index = start_page >> 5;
	size_t bit = start_page & 31;
	size_t dword_masks = pages >> 5;
	for (size_t n = 0; n < dword_masks; ++n)
	{
		_memory_map[dword_index + n] = 0xffffffff;
	}
	bit = 0;
	pages &= 31;
	while (pages--)
	{
		_memory_map[dword_index + dword_masks] |= (1 << bit);
		++bit;
	}
}

vmem_range_t* _vmem_make_range(uintptr_t start, size_t pages)
{
	vmem_range_t* range = (vmem_range_t*)vmem_fixed_alloc(_range_pool, sizeof(vmem_range_t));
	range->_start = start;
	range->_pages = pages;
	return range;
}

vmem_range_t* vmem_allocate(size_t bytes)
{
	// round up to nearest page
	bytes = (bytes + _JOS_KERNEL_PAGE_MOD) & ~_JOS_KERNEL_PAGE_MOD;
	size_t pages = bytes / _JOS_KERNEL_PAGE_SIZE;
	size_t dword_masks = pages >> 5;
	// find a long enough run of free pages
	size_t start_page = 0;
	size_t cont_run = 0;
	for (size_t n = 0; n < _memory_map_dword_count; ++n)
	{
		uint32_t dw = _memory_map[n];
		if (cont_run || dw != 0xffffffff)
		{
			if (!dw)
			{
				if (!cont_run)
					start_page = (n << 5);
				cont_run += 32;
				if (cont_run >= pages)
				{
					return _vmem_make_range(start_page * _JOS_KERNEL_PAGE_SIZE, pages);
				}
			}
			else
			{
				//TODO: use some bitscan or popcount variant instruction here
				for (size_t bit = 0; bit < 32 && (dw & 1) == 0; ++bit)
				{
					cont_run += (dw & 1) ^ 1;
					if (!start_page && cont_run)
					{
						start_page = (n << 5) + bit;
					}
					dw >>= 1;
					if (cont_run >= pages)
					{
						return _vmem_make_range(start_page * _JOS_KERNEL_PAGE_SIZE, pages);
					}
				}
			}
			if (dw)
			{
				cont_run = 0;
				start_page = 0;
			}
		}
	}

	return 0;
}

// a not-very-fast insertion sort to order by size (increasing)
// sorts from a given position down.
_JOS_INLINE_FUNC vmem_range_t* _vmem_range_sort_from(vector_t* bucket, size_t j)
{
	if (!j)
	{
		return (vmem_range_t*)vector_at(bucket, 0);
	}

	while (j)
	{
		vmem_range_t* a = (vmem_range_t*)vector_at(bucket, j);
		vmem_range_t* b = (vmem_range_t*)vector_at(bucket, j - 1);
		if (a->_pages < b->_pages)
		{
			vmem_range_t tmp;
			memcpy(&tmp, a, sizeof(vmem_range_t));
			memcpy(a, b, sizeof(vmem_range_t));
			memcpy(b, &tmp, sizeof(vmem_range_t));
		}
		else
		{
			return a;
			break;
		}
		j--;
	}
	return 0;
}

_JOS_INLINE_FUNC void _vmem_free_range_insert(vmem_range_t range)
{
	size_t i = range._start % kTableSize;
	vector_t* vec = _table + i;
	vector_push_back(vec, &range);
	_vmem_range_sort_from(vec, vector_size(vec) - 1);
}

void vmem_free_range_insert(uintptr_t start, size_t size)
{
	vmem_range_t range;
	range._start = start;
	range._pages = (size / 0x1000) + ((size % 0x1000) ? 1 : 0);
	_vmem_free_range_insert(range);
}

vmem_range_t* vmem_range_allocate(size_t bytes)
{
	size_t pages = (bytes / 0x1000) + ((bytes & 0xfff) ? 1 : 0);
	for (size_t n = 0u; n < kTableSize; ++n)
	{
		size_t size = vector_size(_table + n);
		for (size_t i = 0u; i < size; ++i)
		{
			vmem_range_t* range = (vmem_range_t*)vector_at(_table + n, i);
			if (range->_pages >= pages)
			{
				vmem_range_t alloc_range;
				alloc_range._pages = pages;
				alloc_range._start = range->_start;
				// update free range
				range->_start += pages;
				range->_pages -= pages;
				// the range we've split can never be bigger than the next one in line (as the sequence is already sorted)
				// so we always resort "down", unless this was the first element, in which case the sequence remains sorted
				if (i)
				{
					_vmem_range_sort_from(_table + n, i);
				}
				// now we can insert the new item and re-sort				
				void* ptr = vmem_fixed_alloc(_range_pool, sizeof(vmem_range_t));
				memcpy(ptr, &alloc_range, sizeof(vmem_range_t));
				return ptr;
			}
		}
	}
	return 0;
}

void _vmem_range_internal_alloc(uintptr_t start, size_t bytes)
{
	// must be on a page boundary
	_JOS_ASSERT((start & 0xfff) == 0);

	size_t pages = (bytes / 0x1000) + ((bytes & 0xfff) ? 1 : 0);
	bytes = pages << 12;
	// we now have to find an existing free range that contains the requested range.
	// if we find it then that's the range we will split
	for (size_t n = 0u; n < kTableSize; ++n)
	{
		size_t size = vector_size(_table + n);
		for (size_t i = 0u; i < size; ++i)
		{
			vmem_range_t* range = (vmem_range_t*)vector_at(_table + n, i);
			// contained?
			if (range->_start <= start)
			{
				unsigned long long range_bytes = (unsigned long long)(range->_pages) << 12;
				if (((start + bytes) - range->_start) <= range_bytes)
				{
					vmem_range_t lo = { 0 };
					lo._start = range->_start;
					uintptr_t range_size = (start - range->_start);
					if (range_size)
					{
						lo._pages = (range_size / 0x1000) + ((range_size % 0x1000) ? 1 : 0);
						_vmem_free_range_insert(lo);
					}

					vmem_range_t hi;
					hi._start = start + bytes;
					hi._pages = range->_pages - pages - lo._pages;
					if (hi._pages)
					{
						_vmem_free_range_insert(hi);
					}
					return;
				}
			}
		}
	}
}

void vmem_range_free(vmem_range_t* range)
{
	_vmem_free_range_insert(*range);
	vmem_fixed_free(_range_pool, range);
}

static void _dump_table(void)
{
	for (size_t n = 0u; n < kTableSize; ++n)
	{
		printf("[%d] ", n);
		size_t size = vector_size(_table + n);
		for (size_t i = 0u; i < size; ++i)
		{
			vmem_range_t* range = (vmem_range_t*)vector_at(_table + n, i);
			if (i)
			{
				vmem_range_t* prev_range = (vmem_range_t*)vector_at(_table + n, i - 1);
				_JOS_ASSERT(range->_pages >= prev_range->_pages);
			}
			printf("#");
		}
		printf("\n");
	}
}

static void _check_table(void)
{
	for (size_t n = 0u; n < kTableSize; ++n)
	{
		size_t size = vector_size(_table + n);
		for (size_t i = 0u; i < size; ++i)
		{
			vmem_range_t* range = (vmem_range_t*)vector_at(_table + n, i);
			if (i)
			{
				vmem_range_t* prev_range = (vmem_range_t*)vector_at(_table + n, i - 1);
				if (range->_pages < prev_range->_pages)
				{
					printf("ERROR: invalid table entries at [%d]:%d\n", n, i);
					return;
				}
			}
		}
	}
}

void test_vmap(void)
{
	vmem_init();
	// initialise vm ranges


	_vmem_mark_range(0, 0x100000 / 0x1000);
	_vmem_mark_range(0x101000, 2);
	_vmem_mark_range(0xfec00000, (0xfecfffff - 0xfec00000) / 0x1000);
	_vmem_mark_range(0xfee00000, (0xfeefffff - 0xfee00000) / 0x1000);

	vmem_allocate(102393);
}

// ========================================================================================
// misc

extern int _JOS_LIBC_FUNC_NAME(printf)(const char* __restrict format, ...);
extern int _JOS_LIBC_FUNC_NAME(sprintf_s)(char* __restrict buffer, size_t buffercount, const char* __restrict format, ...);

void test_stdio(void)
{
	char garbage[4] = { 0 };
	garbage[0] = 0xf3;
	garbage[1] = 0x01;
	garbage[3] = 0xcc;
	_JOS_LIBC_FUNC_NAME(printf)("hello world %s!\n", garbage);
}


extern void _k_trace(const char* __restrict channel, const char* __restrict format, ...);

#undef _JOS_KTRACE
#define _JOS_KTRACE(channel, msg,...) _k_trace(channel, msg,##__VA_ARGS__)
static const char* kTestChannel = "test";

void test_ktrace(void)
{
	_JOS_KTRACE(kTestChannel, "b b boooo?\n");
	uint8_t _bus_id = 1;
	uint8_t bus_id[6];
	sprintf_s(bus_id, 6, "%s", "ISA");
	_JOS_KTRACE(kTestChannel, "bus %d, %s\n", _bus_id, bus_id);
	char garbage[4] = { 0 };
	garbage[0] = 0xf3;
	garbage[1] = 0x01;
	garbage[3] = 0xcc;
	_k_trace(kTestChannel, "hello %s!\n", garbage);
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
	if (isVMM)
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
	char sig[4] = { 'a','b','c','d' };
	_JOS_LIBC_FUNC_NAME(printf)("test line %4s...\n", sig);
	for (int n = 0; n < 50; ++n)
	{
		_JOS_LIBC_FUNC_NAME(printf)("line\t%d\n", n);
		vga_draw();
	}
	_JOS_LIBC_FUNC_NAME(printf)("last line");
}

enum hex_dump_unit_size
{
	k8bitInt,
	k16bitInt,
	k32bitInt,
	k64bitInt,
};


typedef struct _line_ctx
{
	char		_line[128];
	char*		_wp;
	size_t		_chars_left;
	void*		_mem;
	
} line_ctx_t;

static void _mem_dump_line_init(line_ctx_t* ctx, void* mem)
{
	ctx->_wp = ctx->_line;
	ctx->_mem = mem;
	ctx->_chars_left = sizeof(ctx->_line);	
	// address prefix
	size_t n = _JOS_LIBC_FUNC_NAME(sprintf_s)(ctx->_wp, ctx->_chars_left,"%08x ", (uintptr_t)mem);
	ctx->_wp += n;
	ctx->_chars_left -= n;
}

static void _mem_dump_line_write_byte(line_ctx_t* ctx, unsigned char byte, int fmtIdx)
{
	static const char* kFmt[2] = {"%02x ", "%02x"};
	size_t n = _JOS_LIBC_FUNC_NAME(sprintf_s)(ctx->_wp, ctx->_chars_left, kFmt[fmtIdx], byte);	
	ctx->_wp += n;
	ctx->_chars_left -= n;
}

static void _mem_dump_line_write_word(line_ctx_t* ctx, unsigned short word)
{
	size_t n = _JOS_LIBC_FUNC_NAME(sprintf_s)(ctx->_wp, ctx->_chars_left, "%04x ", word);
	ctx->_wp += n;
	ctx->_chars_left -= n;
}

static void _mem_dump_line_write_dword(line_ctx_t* ctx, unsigned int dword)
{
	if(dword == 0xc168108)
		__debugbreak();
	size_t n = _JOS_LIBC_FUNC_NAME(sprintf_s)(ctx->_wp, ctx->_chars_left, "%08lx ", dword);
	ctx->_wp += n;
	ctx->_chars_left -= n;
}

static void _mem_dump_line_write_qword(line_ctx_t* ctx, unsigned long long qword)
{
	size_t n = _JOS_LIBC_FUNC_NAME(sprintf_s)(ctx->_wp, ctx->_chars_left, "%016llx ", qword);
	ctx->_wp += n;
	ctx->_chars_left -= n;
}

static void _mem_dump_line_write_raw(line_ctx_t* ctx, size_t byte_run)
{
	// right aligned	
	memset(ctx->_wp, ' ', ctx->_chars_left);	
	char* wp = ctx->_line + 58;
	ctx->_chars_left = sizeof(ctx->_line) - 58;
	const unsigned char* rp = (unsigned char*)ctx->_mem;
	for (unsigned i = 0u; i < byte_run; ++i)
	{
		const char c = (char)*rp++;
		size_t n;
		if(c >= 32 && c < 127)
		{
			n = _JOS_LIBC_FUNC_NAME(sprintf_s)(wp, ctx->_chars_left, "%c", c);
		}
		else
		{
			n = _JOS_LIBC_FUNC_NAME(sprintf_s)(wp, ctx->_chars_left, ".");
		}
		ctx->_chars_left -= n;
		wp += n;
	}
}

static size_t _mem_dump_hex_line(void* mem, size_t bytes, enum hex_dump_unit_size unit_size)
{
	line_ctx_t ctx;
	size_t read = 0;	
	
	if (bytes)
	{				
		_mem_dump_line_init(&ctx, (uintptr_t)mem);		
		const unsigned byte_run = min(bytes, 16);

		switch (unit_size)
		{
		case k8bitInt:
		{
			char* rp = (char*)mem;
			for (unsigned i = 0u; i < byte_run; ++i)
			{
				_mem_dump_line_write_byte(&ctx, (unsigned char)*rp++,0);
				++read;
			}
		}
		break;
		case k16bitInt:
		{
			unsigned short* rp = (unsigned short*)mem;
			unsigned run = min(byte_run/2, 8);
			for (unsigned i = 0u; i < run; ++i)
			{
				_mem_dump_line_write_word(&ctx, *rp++);
				read+=sizeof(unsigned short);
			}
			if ( byte_run & 1 )
			{
				run = byte_run & 1;
				for (unsigned i = 0u; i < run; ++i)
				{
					_mem_dump_line_write_byte(&ctx, (unsigned char)*rp++, 1);
					++read;
				}
			}
		}
		break;
		case k32bitInt:
		{
			unsigned int* rp = (unsigned int*)mem;
			unsigned run = min(byte_run/4, 4);
			for (unsigned i = 0u; i < run; ++i)
			{
				_mem_dump_line_write_dword(&ctx, *rp++);
				read+=sizeof(unsigned int);
			}
			if ( byte_run & 3 )
			{
				run = byte_run & 3;
				for (unsigned i = 0u; i < run; ++i)
				{
					_mem_dump_line_write_byte(&ctx, (unsigned char)*rp++, 1);
					++read;
				}
			}
		}
		break;
		case k64bitInt:
		{
			unsigned long long* rp = (unsigned long long*)mem;
			unsigned run = min(byte_run/8, 2);
			for (unsigned i = 0u; i < run; ++i)
			{
				_mem_dump_line_write_qword(&ctx, *rp++);
				read+=sizeof(unsigned long long);
			}
			if ( byte_run & 7 )
			{
				run = byte_run & 7;
				for (unsigned i = 0u; i < run; ++i)
				{
					_mem_dump_line_write_byte(&ctx, (unsigned char)*rp++, 1);
					++read;
				}
			}
		}
		break;
		default:;
		}

		_mem_dump_line_write_raw(&ctx, byte_run);
	}
	
	if(read)
	{				
		_JOS_LIBC_FUNC_NAME(printf)("%s\n",ctx._line);
	}
	return read;
}



void mem_dump_hex(void* mem, size_t bytes, enum hex_dump_unit_size unit_size)
{	
	while (bytes)
	{
		size_t written = _mem_dump_hex_line(mem, bytes, unit_size);		
		mem = (void*)((char*)mem + written);
		bytes -= written;
	}
	vga_draw();
}

void test_hexdump()
{
	srand(101);
	static const size_t kSomeMemorySize = 49;
	void* some_memory = malloc(kSomeMemorySize);
	char* s = (char*)some_memory;	
	int n = 1 + sprintf_s(s, kSomeMemorySize, "this is a string\n");
	for(;n<kSomeMemorySize;++n)
	{
		s[n] = (char)(rand() & 0xff);
	}
	mem_dump_hex(some_memory, kSomeMemorySize, k8bitInt);
	printf("\n");
	mem_dump_hex(some_memory, kSomeMemorySize, k16bitInt);
	printf("\n");
	mem_dump_hex(some_memory, kSomeMemorySize, k32bitInt);
	printf("\n");
	mem_dump_hex(some_memory, kSomeMemorySize, k64bitInt);

	free(some_memory);
}

int main(void)
{
	init_console();
	//test_console();
	
	test_hexdump();
 	test_hypervisor();
	test_vector();
	test_mem();
	test_queue();
	test_stdio();
	test_ktrace();
	test_vmap();

	output_console_destroy(&_stdout);
	return 0;
}