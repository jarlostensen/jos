
#include <stdlib.h>
#include <kernel/kernel.h>

// https://linux.die.net/man/3/malloc

void *malloc(size_t size)
{
    if(!size)
        return 0;
    return k_mem_alloc(size);
}

void free(void *ptr)
{
    if(!ptr)
        return;
    k_mem_free(ptr);
}

void *calloc(size_t nmemb, size_t size)
{
    if(!nmemb || !size)
        return 0;
    return k_mem_alloc(nmemb * size);
}

void *realloc(void *ptr, size_t size)
{
    if(!ptr)
        return malloc(size);
    
    if(!size)
    {
        free(ptr);
        return ptr;
    }

    //TODO: built in realloc, using knowledge of size of allocation
    void* new_ptr = k_mem_alloc(size);
    if(new_ptr)
        k_mem_free(ptr);
    return new_ptr;
}
