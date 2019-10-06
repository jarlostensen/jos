#ifndef JOS_KERNEL_DETAIL_H
#define JOS_KERNEL_DETAIL_H

#include <stdint.h>
#include <stddef.h>

// interrupt service routines
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr32();

// ia-32 dev manual 6-12
struct isr_stack_struct
{    
    // pushed by isr handler stub (pushad)
    uint32_t    edi, esi, ebp, esp, ebx, edx, ecx, eax;
    // pushed by CPU
    uint32_t    error_code;
    uint32_t    eip;
    uint32_t    cs;
    uint32_t    eflags;
    // iff privilege level switch
    uint32_t    _esp;
    uint32_t    _ss;
};
typedef struct isr_stack_struct isr_stack_t;

typedef void (*isr_handler_func_t)(isr_stack_t isr_stack);

extern void enable_interrupts();
extern void disable_interrupts();
extern void halt_cpu();

typedef enum alignment_enum
{
    kNone = 0,
    k16 = 2,
    k32 = 4,
    k64 = 8,
    k128 = 16,
    k256 = 32,
    k512 = 64,
    k4k = 0x1000,
} alignment_t;
void* kalloc(size_t bytes, alignment_t alignment);
void kalloc_init();

void kernel_panic();

#endif // JOS_KERNEL_DETAIL_H