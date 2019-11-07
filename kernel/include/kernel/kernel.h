
#ifndef JOS_KERNEL_H
#define JOS_KERNEL_H

#include <stdint.h>
#include "tty.h"

//TODO: some (or most) of these should eventually end up in libc, probably as a master "jos.h" in the sys include folder?

#define JOS_BOCHS_DBGBREAK() asm("xchg %bx,%bx")
#define JOS_ASSERT(cond) \
    if (!(cond))\
    {\
        asm("xchg %bx,%bx");\
        asm("int $0x3");\
    }

void k_outb(uint16_t port, uint8_t value);
void k_outw(uint16_t port, uint16_t value);
uint8_t k_inb(uint16_t port);
uint16_t k_inw(uint16_t port);
// pause the current core (for spin waits)
#define k_pause(void)\
asm volatile ("pause")

// read TSC
inline uint64_t __rdtsc()
{
    uint64_t ret;
    asm volatile ( "rdtsc" : "=A"(ret) );
    return ret;
}

void _k_trace(const char* msg,...);
#define JOS_KTRACE(msg,...) _k_trace(msg,##__VA_ARGS__)

void _k_trace_buf(const void* data, size_t length);
#define JOS_KTRACE_BUF(data,length) _k_trace_buf(data, length)

#endif // JOS_KERNEL_H