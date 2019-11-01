
#include "kernel_detail.h"
#include <stdio.h>

// in link.ld
void _k_phys_end(void);
void _k_phys_start(void);

static uint32_t _free_ptr = 0;
static size_t _allocated = 0;

void _k_alloc_init()
{
    _free_ptr = (uint32_t)&_k_phys_end;
    printf("_k_alloc_init. Kernel is %d bytes, heap starts at 0x%x\n", (uint32_t)&_k_phys_end - (uint32_t)&_k_phys_start, _free_ptr);
}

void* _k_alloc(size_t bytes, alignment_t alignment)
{
    switch(alignment)
    {
        case kAlign16:
        {
            _allocated += _free_ptr & 0x1;
            _free_ptr = (_free_ptr + 1) & ~1;
        }
        break;
        case kAlign32:
        {
            _allocated += _free_ptr & 0x3;
            _free_ptr = (_free_ptr + 3) & ~3;
        }
        break;
        case kAlign64:
        {
            _allocated += _free_ptr & 0x7;
            _free_ptr = (_free_ptr + 7) & ~7;
        }
        break;
        case kAlign128:
        {
            _allocated += _free_ptr & 0xf;
            _free_ptr = (_free_ptr + 0xf) & ~0xf;
        }
        break;
        case kAlign256:
        {
            _allocated += _free_ptr & 0x1f;
            _free_ptr = (_free_ptr + 0x1f) & ~0x1f;
        }
        break;
        case kAlign512:
        {
            _allocated += _free_ptr & 0x1ff;
            _free_ptr = (_free_ptr + 0x1ff) & ~0x1ff;
        }
        break;
        case kAlign4k:
        {
            _allocated += _free_ptr & 0xfff;
            _free_ptr = (_free_ptr + 0xfff) & ~0xfff;
        }
        break;
        default:;
    }
    uint32_t ptr = _free_ptr;
    _free_ptr += bytes;
    _allocated += bytes;
    return (void*)ptr;
}
