#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../include/kernel/kernel.h"
#include "../include/kernel/tty.h"

#include "kernel_detail.h"
#include "dt.h"
#include "../arch/i386/vga.h"

// =======================================================

gdt_entry_t _gdt[5] = {
    // null
    { .limit_low = 0, .base_low = 0, .base_middle = 0, .access.byte = 0, .granularity.byte = 0, .base_high = 0 },
    // kernel code
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.byte = 0b10011010, .granularity.fields = { .limit_high = 0xf, .size = 1, .granularity = 1 }, .base_high = 0 },
    // kernel data
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.byte = 0b10010010, .granularity.byte = 0b11001111, .base_high = 0 },
    // user code
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.fields = { .rw = 1,  .executable = 1, .one=1, .privilege=3, .present = 1 }, .granularity.fields = { .limit_high = 0xf, .size = 1, .granularity = 1 }, .base_high = 0 },
    // user data
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.fields = { .rw = 1, .one=1, .privilege=3, .present = 1 }, .granularity.byte = 0b11001111, .base_high = 0 },
};
gdt32_descriptor_t _gdt_desc = {.size = sizeof(_gdt), .address = (uint32_t)(_gdt)};

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

//TODO: move this somewhere else, make isr.c or something
void isr3_handler(isr_stack_t stack)
{
    JOS_BOCHS_DBGBREAK();
    printf("isr3_handler, error code is %x\n", stack.error_code);
}

void init_isrs()
{
    idt_set_gate(0,0x08,0,&(idt_type_attr_t){.gate_type = 1, .storage_segment = 0, .dpl = 3, .present = 1},isr3_handler);
    idt_set_gate(0,0x08,0,&(idt_type_attr_t){.gate_type = 1, .storage_segment = 0, .dpl = 3, .present = 1},isr3_handler);
    idt_set_gate(0,0x08,0,&(idt_type_attr_t){.gate_type = 1, .storage_segment = 0, .dpl = 3, .present = 1},isr3_handler);
}

void kernel_panic()
{
    terminal_set_colour(vga_entry_color(VGA_COLOR_WHITE,VGA_COLOR_LIGHT_BLUE));
    terminal_writestring("KERNEL PANIC!");
    halt_cpu();
}

void _kinit(void *mboot)
{     
    kalloc_init();
    terminal_initialize();
    terminal_disable_cursor();
    printf("_kinit(0x%x), %s\n", (int)mboot, is_protected_mode() ? "protected mode":"real mode");    

    void* allocated = kalloc(1024, kNone);
    memset(allocated, 0xdd, 1024);
    printf(" allocated and cleared memory at 0x%x\n", (int)allocated);
}

void _kmain()
{
    printf("_kmain %s\n", is_protected_mode() ? "protected mode":"real mode");    
    kernel_panic();
}