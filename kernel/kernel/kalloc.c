
#include "kernel_detail.h"

extern uint32_t _end;
static uint32_t _free_ptr;
static size_t _allocated = 0;

void kalloc_init()
{
    _free_ptr = _end;
}

void* kalloc(size_t bytes, alignment_t alignment)
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
