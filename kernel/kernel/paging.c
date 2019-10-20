
#include "../include/kernel/kernel.h"
#include "interrupts.h"
#include "kernel_detail.h"

// https://wiki.osdev.org/Paging

struct page_directory_flags_struct
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
} __attribute__((packed));
typedef struct page_directory_flags_struct page_directory_flags_t;
struct page_directory_struct 
{
    union 
    {
        page_directory_flags_t  _flags;
        uint32_t                _phys_address;
    } _entry;
}  __attribute__((packed));
typedef struct page_directory_struct page_directory_t;

struct page_table_flags_struct
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
} __attribute__((packed));
typedef page_table_flags_struct page_table_flags_t;
struct page_table_struct 
{
    union 
    {
        page_table_flags_t  _flags;
        uint32_t            _phys_address;
    } _entry;
}  __attribute__((packed));
typedef struct page_table_struct page_table_t;

extern void _k_load_page_directory(uint32_t physPageDirStart);

void k_paging_identity_map(page_directory_t* pg, uint32_t start, size_t size)
{
    page_table_t * pt = (page_table_t*)pg->_entry._phys_address;
    start &= 0xfffff000;
    while(size)
    {
        pt->_entry._phys_address = start;
        pt->_entry._flags._present = 1;
        pt++;
        size -= 0x1000;
    }
}