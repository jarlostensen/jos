
#include "kernel_detail.h"
#include <stdio.h>

extern uint32_t _end;
static uint32_t _free_ptr;
static size_t _allocated = 0;

void _k_alloc_init()
{
    _free_ptr = _end;
    printf("_k_alloc_init. Kernel heap starts at 0x%x\n", _free_ptr);
}

void* _k_alloc(size_t bytes, alignment_t alignment)
{
    switch(alignment)
    {
        //TODO:
        default:;
    }
    uint32_t ptr = _free_ptr;
    _free_ptr += bytes;
    _allocated += bytes;
    return (void*)ptr;
}
