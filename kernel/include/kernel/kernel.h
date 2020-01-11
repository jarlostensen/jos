
#ifndef JOS_KERNEL_H
#define JOS_KERNEL_H

#include <stdint.h>

#define JOS_BOCHS_DBGBREAK() asm volatile ("xchg %bx,%bx")
#define JOS_ASSERT(cond) if(!(cond)) {asm volatile ("xchg %bx,%bx");}

// ====================================================================================
// tracing 
void _k_trace(const char* msg,...);
#define JOS_KTRACE(msg,...) _k_trace(msg,##__VA_ARGS__)

void _k_trace_buf(const void* data, size_t length);
#define JOS_KTRACE_BUF(data,length) _k_trace_buf(data, length)

// ====================================================================================
// memory

// return physical address or 0 (which is of course valid if virt is 0...)
uintptr_t k_mem_virt_to_phys(uintptr_t virt);

// return the page table entry for the frame containing virt
uintptr_t _k_mem_virt_to_pt_entry(uintptr_t virt);

enum k_mem_valloc_flags_enum
{
    kMemValloc_Reserve = 1,
    kMemValloc_Commit,
};
// allocate size bytes of virtual memory, or 0 if not enough available
void* k_mem_valloc(size_t size, int flags);

void* k_mem_alloc(size_t size);
void k_mem_free(void* ptr);

// ====================================================================================
// I/O

void k_outb(uint16_t port, uint8_t value);
void k_outw(uint16_t port, uint16_t value);
uint8_t k_inb(uint16_t port);
uint16_t k_inw(uint16_t port);

// ====================================================================================
// misc control

__attribute__((__noreturn__)) void k_panic();
// pause the current core (for spin waits)
#define k_pause(void)\
asm volatile ("pause")

// ====================================================================================
// architecture

// read TSC
inline uint64_t __rdtsc()
{
    uint64_t ret;
    asm volatile ( "rdtsc" : "=A"(ret) );
    return ret;
}

#endif // JOS_KERNEL_H