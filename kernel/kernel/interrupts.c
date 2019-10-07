#include "kernel_detail.h"
#include "../include/kernel/kernel.h"
#include "dt.h"
#include "interrupts.h"

#include <stdio.h>
#include <string.h>

// in arch/i386/interrupts.asm
extern void k_load_idt(void);

idt_entry_t _idt[256];
idt32_descriptor_t _idt_desc = {.size = sizeof(_idt), .address = (uint32_t)(_idt)};
isr_handler_func_t _isr_handlers[256];

static void idt_set_gate(uint8_t i, uint16_t sel, uint32_t offset, idt_type_attr_t* type_attr, isr_handler_func_t handler)
{
    idt_entry_t* entry = _idt + i;
    entry->offset_high = (offset >> 16) & 0xffff;
    entry->offset_low = offset & 0xffff;
    entry->selector = sel;
    entry->type_attr.fields = *type_attr;
    _isr_handlers[i] = handler;
    //printf("idt_set_gate for %x", i);
}

void _isr_handler(int isr_id)
{
    JOS_BOCHS_DBGBREAK();
    printf("isr_handler %d\n", isr_id);
}

void k_init_isrs()
{
    memset(_idt, 0, sizeof(_idt));

#define K_ISR_SET(i)\
    idt_set_gate(i,K_CODE_SELECTOR,0,&(idt_type_attr_t){.gate_type = 0xe, .dpl = 3, .present = 1}, isr##i)

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

    // make it so!
    k_load_idt();
}
