#include <stdint.h>
#include "../include/kernel/kernel.h"
#include "dt.h"

// =======================================================

gdt_entry_t _gdt[3] = {
    // null
    { .limit_low = 0, .base_low = 0, .base_middle = 0, .access.byte = 0, .granularity.byte = 0, .base_high = 0 },
    // code
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.byte = 0b10011010, .granularity.fields = { .limit_high = 0xf, .zero = 0, .size = 1, .granularity = 1 }, .base_high = 0 },
    // data
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.byte = 0b10010010, .granularity.byte = 0b11001111, .base_high = 0 },
};
gdt32_descriptor_t _gdt_desc = {.size = sizeof(_gdt), .address = (uint32_t)(_gdt)};

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

idt_entry_t _idt[256];
idt32_descriptor_t _idt_desc = {.size = sizeof(_idt), .address = (uint32_t)(_idt)};

static void idt_set_gate(uint8_t i, uint16_t sel, uint32_t offset, idt_type_attr_t* type_attr)
{
    idt_entry_t* entry = _idt + i;
    entry->offset_high = (offset >> 16) & 0xffff;
    entry->offset_low = offset & 0xffff;
    entry->selector = sel;
    //zzz:
    entry->type_attr.fields = *type_attr;
}

// =======================================================================
// Misc TTY
static const uint32_t kVgaMemBase = 0xb8000;
static const uint32_t kVgaMemWords = 80*25;
static const uint32_t kVgaMemEnd = kVgaMemBase + kVgaMemWords*2;

void clear_vga(char attr)
{
    char* vga_ptr = (char*)(kVgaMemBase);
    for(unsigned c = 0; c < kVgaMemWords<<1; ++c)
    {
        vga_ptr[0] = 0x20;
        vga_ptr[1] = attr;
        vga_ptr+=2;
    }
}

void write_to_vga(char* str, char attr, char x, char y)
{
    char* vga_ptr = (char*)(kVgaMemBase + x + y*(80*2));    
    while(*str &&  (uint32_t)(vga_ptr)<kVgaMemEnd)
    {
        vga_ptr[0] = *str++;
        vga_ptr[1] = attr;
        vga_ptr+=2;
    }
}

void _kinit(void *mboot)
{     
    (void)mboot;
    clear_vga(0x1c);
    write_to_vga("Initialising kernel", 0x1c, 0,0);         

    idt_set_gate(0,0x08,0,&(idt_type_attr_t){.gate_type = 1, .storage_segment = 0, .dpl = 3, .present = 1});
}

void _kmain()
{
    //_bochs_debugbreak();
    write_to_vga("_kmain, we're in protected mode", 0x2a, 0,3);       
}