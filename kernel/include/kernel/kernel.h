
#ifndef JOS_KERNEL_H
#define JOS_KERNEL_H

#include <stdint.h>
#include "tty.h"

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
// pause the current core (if applicable and available)
// for spin waits
void k_pause(void);

void _k_trace(const char* msg);
#define JOS_KTRACE(msg) _k_trace(msg)

#endif // JOS_KERNEL_H