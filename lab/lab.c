
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
	}
	assert(queue_is_full(&queue));

	for(uintptr_t n = 0; n < 100; ++n)
	{
		queue_pop(&queue);
		queue_push(&queue,&n);
		assert(queue_is_full(&queue));
	}

	queue_destroy(&queue);
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

#define TTY_HEIGHT 8
#define TTY_WIDTH 25

typedef struct _console_output_driver
{
	// assumes stride in bytes
	void (*_blt)(void* src, size_t start_line, size_t stride, size_t width, size_t lines);
	void (*_clear)(uint16_t value);
} console_output_driver_t;

void _printf_driver_blt(void * src_, size_t start_line, size_t stride, size_t width, size_t lines)
{
	stride >>= 1;
	const int output_width = min(TTY_WIDTH, width);
	lines = min(lines, TTY_HEIGHT - start_line);
	uint16_t* src = (uint16_t*)src_;

	while(lines--)
	{
		for(int c = 0; c < output_width; ++c)
		{
			printf("%c", (char)src[c]);
		}
		if ( lines )
			printf("\n");
		src += stride;
	}
}

typedef struct _output_console
{
	uint16_t		_rows;
	uint16_t		_columns;
	int16_t			_start_row;
	int16_t			_col;
	int16_t			_row;
	//NOTE: buffer includes attribute byte
	uint16_t*		_buffer;

	console_output_driver_t* _driver;

} output_console_t;

void output_console_create(output_console_t* con)
{
	_JOS_ASSERT(con);
	_JOS_ASSERT(con->_rows);
	_JOS_ASSERT(con->_columns);
	const int buffer_byte_size = con->_rows*con->_columns*sizeof(uint16_t);
	con->_buffer = (uint16_t*)malloc(buffer_byte_size);
	con->_start_row = 0;
	con->_col = con->_row = 0;
	memset(con->_buffer, '.', buffer_byte_size);
}

void output_console_destroy(output_console_t* con)
{
	if(!con || !con->_rows || !con->_columns || !con->_buffer)
		return;

	free(con->_buffer);
	memset(con,0,sizeof(output_console_t));
}

void output_console_flush(output_console_t* con)
{
#ifndef _JOS_KERNEL_BUILD
	printf("\n");
	for(int c = 0; c < TTY_WIDTH; ++c)
	{
		printf("-");
	}
	printf("\n");
#endif

	_JOS_ASSERT(con);
	_JOS_ASSERT(con->_buffer);
	const int output_width = min(con->_columns, TTY_WIDTH);
	const size_t stride = (size_t)con->_columns*2;

	if(con->_start_row < con->_row)
	{
		int lines_to_flush = min(con->_row+1, TTY_HEIGHT);
		const int flush_start_row = con->_row - min(TTY_HEIGHT-1, con->_row-con->_start_row);
		//NOTE: always flush from col 0
		con->_driver->_blt(con->_buffer + flush_start_row * con->_columns, 0, stride, (size_t)output_width, lines_to_flush);
	}
	else
	{
		const int lines_in_buffer = con->_row + (con->_rows - con->_start_row) + 1; //< +1 to convert _row to count
		int lines_to_flush = min(lines_in_buffer, TTY_HEIGHT);
		int flush_row = con->_rows - (lines_in_buffer - con->_row - 1); //< as above

		//NOTE: always flush from col 0
		uint16_t cc = flush_row * con->_columns;		
		const size_t lines = con->_rows - flush_row - 1;
		con->_driver->_blt(con->_buffer + cc, 0, stride, (size_t)output_width, lines);
		cc += lines*con->_columns;
#ifndef _JOS_KERNEL_BUILD
	printf("\n");
#endif
		con->_driver->_blt(con->_buffer + cc, lines, stride, (size_t)output_width, con->_row + 1);
	}
}

void output_console_println(output_console_t* con)
{
	_JOS_ASSERT(con);
	_JOS_ASSERT(con->_buffer);
	con->_row = (con->_row+1) % con->_rows;
	if(con->_row == con->_start_row)
	{
		con->_start_row = (con->_start_row+1) % con->_rows;
	}
	con->_col = 0;
	//ZZZ:
	memset(con->_buffer+(con->_row*con->_columns), '.', con->_columns * sizeof(uint16_t));
}

void output_console_print(output_console_t* con, const char* line, char attr)
{
	_JOS_ASSERT(con);
	_JOS_ASSERT(con->_buffer);

	//TODO: horisontal scroll
	const int output_width = min(strlen(line), con->_columns);
	uint16_t cc = con->_row*con->_columns + con->_col;
	const uint16_t attr_mask = ((uint16_t)attr << 8);
	for(int c = 0; c < output_width; ++c)
	{
		if(line[c]=='\n')
		{
			output_console_println(con);
			cc = con->_row*con->_columns;
		}
		else 
		{
			con->_buffer[cc++] = attr_mask | line[c];		
			++con->_col;
		}
	}
}

void test_console(void)
{
	console_output_driver_t driver;
	driver._blt = _printf_driver_blt;

	output_console_t con;
	con._columns = 100;
	con._rows = TTY_HEIGHT-1;
	con._driver = &driver;
	output_console_create(&con);
	
	for(int n = 0; n < (2*TTY_HEIGHT+1); ++n)
	{
		char buffer[200];
		output_console_print(&con, "line ", 0x20);
		sprintf_s(buffer, sizeof(buffer), "%d\n", n+1);
		output_console_print(&con, buffer, 0x20);
		output_console_flush(&con);
	}
	output_console_print(&con, "last line", 0x20);
	output_console_flush(&con);
	
 	output_console_destroy(&con);
	printf("\n");
}

int main()
{
	test_console();

	test_hypervisor();
	test_vector();
	test_mem();
	test_queue();
	test_stdio();
	test_ktrace();
	return 0;
}