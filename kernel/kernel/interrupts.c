#include "kernel_detail.h"
#include "../include/kernel/kernel.h"
#include "dt.h"
#include "interrupts.h"

#include <stdio.h>
#include <string.h>


// ==============================================================================
// IDT
// https://wiki.osdev.org/Interrupt_Descriptor_Table

struct idt_type_attr_struct
{
    /* 
    only care about either of these:
    0x5 : 80386 32 bit task gate
    0xe : 80386 32-bit interrupt gate
    0xf : 80386 32-bit trap gate
    */
    uint8_t gate_type : 4;
    // 0 for interrupt and trap gates
    uint8_t storage_segment : 1;
    // Descriptor Privilege Level
    uint8_t dpl : 2;
    // 0 for unused interrupts
    uint8_t present:1;
};
typedef struct idt_type_attr_struct idt_type_attr_t;

struct idt_entry_struct
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    union 
    {
        uint8_t byte;
        idt_type_attr_t fields;
    } type_attr;
    uint16_t offset_high;
} __attribute((packed));
typedef struct idt_entry_struct idt_entry_t;

struct idt32_descriptor_struct 
{
    uint16_t size;
    uint32_t address;
} __attribute((packed));
typedef struct idt32_descriptor_struct idt32_descriptor_t;

// interrupt service routines
extern void _k_isr0();
extern void _k_isr1();
extern void _k_isr2();
extern void _k_isr3();
extern void _k_isr4();
extern void _k_isr5();
extern void _k_isr6();
extern void _k_isr7();
extern void _k_isr8();
extern void _k_isr9();
extern void _k_isr10();
extern void _k_isr11();
extern void _k_isr12();
extern void _k_isr13();
extern void _k_isr14();
extern void _k_isr15();
extern void _k_isr16();
extern void _k_isr17();
extern void _k_isr18();
extern void _k_isr19();
extern void _k_isr20();
extern void _k_isr21();
extern void _k_isr22();
extern void _k_isr23();
extern void _k_isr24();
extern void _k_isr25();
extern void _k_isr26();
extern void _k_isr27();
extern void _k_isr28();
extern void _k_isr29();
extern void _k_isr30();
extern void _k_isr31();
extern void _k_isr32();

// ia-32 dev manual 6-12
struct isr_stack_struct
{    
    // pushed by isr handler stub
    uint32_t    ds;
    uint32_t    edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t    handler_id;
    
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


// in arch/i386/interrupts.asm
extern void _k_load_idt(void);
extern void _k_store_idt(idt32_descriptor_t* desc);

idt_entry_t _idt[256];
idt32_descriptor_t _idt_desc = {.size = sizeof(_idt), .address = (uint32_t)(&_idt)};
isr_handler_func_t _isr_handlers[256];

static void idt_set_gate(uint8_t i, uint16_t sel, uint32_t offset, idt_type_attr_t* type_attr)
{
    idt_entry_t* entry = _idt + i;
    entry->offset_high = (uint16_t)((offset >> 16) & 0x0000ffff);
    entry->offset_low = offset & 0xffff;
    entry->selector = sel;
    entry->type_attr.fields = *type_attr;
}

isr_handler_func_t k_set_isr_handler(int i, isr_handler_func_t handler)
{
    isr_handler_func_t prev = _isr_handlers[i];    
    _isr_handlers[i] = handler;
    printf("k_set_isr_handler 0x%x, prev = 0x%x, new = 0x%x\n", i, prev, handler);
    return prev;
}

void _isr_handler(isr_stack_t isr_stack)
{
    if(_isr_handlers[isr_stack.handler_id])
    {
        _isr_handlers[isr_stack.handler_id](isr_stack.cs, isr_stack.eip);
        return;
    }
    // no handler
    printf("_isr_handler: unhandled interrupt 0x%x, next instruction @ 0x%x : 0x%x\n", isr_stack.handler_id, isr_stack.cs, isr_stack.eip);
}

void k_init_isrs()
{
    memset(_idt, 0, sizeof(_idt));
    memset(_isr_handlers, 0, sizeof(_isr_handlers));
    printf("k_init_irs: _idt_desc.size = %d, _idt_desc.address = 0x%x\n", (int)_idt_desc.size, _idt_desc.address);

#define K_ISR_SET(i)\
    idt_set_gate(i,K_CODE_SELECTOR,(uint32_t)_k_isr##i,&(idt_type_attr_t){.gate_type = 0xe, .dpl = 0, .present = 1})

    K_ISR_SET(0);
    K_ISR_SET(1);
    K_ISR_SET(2);
    K_ISR_SET(3);
    K_ISR_SET(4);
    K_ISR_SET(5);
    K_ISR_SET(6);
    K_ISR_SET(7);
    K_ISR_SET(8);
    K_ISR_SET(9);
    K_ISR_SET(10);
    K_ISR_SET(11);
    K_ISR_SET(12);
    K_ISR_SET(13);
    K_ISR_SET(14);
    K_ISR_SET(15);
    K_ISR_SET(16);
    K_ISR_SET(17);
    K_ISR_SET(18);
    K_ISR_SET(19);
    K_ISR_SET(20);
    K_ISR_SET(21);
    K_ISR_SET(22);
    K_ISR_SET(23);
    K_ISR_SET(24);
    K_ISR_SET(25);
    K_ISR_SET(26);
    K_ISR_SET(27);
    K_ISR_SET(28);
    K_ISR_SET(29);
    K_ISR_SET(30);
    K_ISR_SET(31);
}

void k_load_isrs()
{
    //TODO: some error checking?
    // make it so!
    _k_load_idt(); 
}
