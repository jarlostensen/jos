#pragma once
#ifndef JOS_KERNEL_DT_H
#define JOS_KERNEL_DT_H
#include <stdint.h>

// ==============================================================================
// GDT
// https://wiki.osdev.org/Global_Descriptor_Table
// http://www.independent-software.com/operating-system-development-protected-mode-global-descriptor-table.html

struct gdt_access_entry_struct
{
    // 0 The CPU will set this to 1 when the segment is accessed. Initially, set to 0.
    uint8_t accessed:1;
    // 1 (code segments)	If set to 1, then the contents of the memory can be read. It is never allowed to write to a code segment.
    // 1 (data segments)	If set to 1, then the contents of the memory can be written to. It is always allowed to read from a data segment.
    uint8_t rw:1;
    // 2 (data segments) "direction" A value of 1 indicates that the segment grows down, while a value of 0 indicates that it grows up. If a segment grows down, then the offset has to be greater than the base. You would normally set this to 0.
    // 2 (code segments) "conforming" A value of 1 indicates that the code can be executed from a lower privilege level. If 0, the code can only be executed from the privilege level indicated in the Privl flag.    
    uint8_t dc:1;
    // 3 If set to 1, the contents of the memory area are executable code. If 0, the memory contains data that cannot be executed.
    uint8_t executable:1;
    // 4 always 1
    uint8_t one:1;
    // 5
    // There are four privilege levels of which only levels 0 and 3 are relevant to us.
    //   Code running at level 0 (kernel code) has full privileges to all processor instructions, while code with level 3 has access to a limited set (user programs). 
    //   This is relevant when the memory referenced by the descriptor contains executable code.
    uint8_t privilege:2;
    // 7 selectors can be marked as “not present” so they can’t be used. Normally, set it to 1.
    uint8_t present:1;
};
typedef struct gdt_access_entry_struct gdt_access_entry_t;

struct gdt_granularity_entry_struct
{
    uint8_t limit_high : 4;
    uint8_t zero : 2;
    uint8_t size:1;
    uint8_t granularity:1;
};
typedef struct gdt_granularity_entry_struct gdt_granularity_entry_t;

struct gdt_entry_struct
{
   uint16_t limit_low;           // The lower 16 bits of the limit.
   uint16_t base_low;            // The lower 16 bits of the base.
   uint8_t  base_middle;         // The next 8 bits of the base.
   union 
   {
    uint8_t  byte;
    gdt_access_entry_t fields;
   } access;
   union 
   {
    uint8_t byte;
    gdt_granularity_entry_t fields;
   } granularity;
   uint8_t  base_high;           // The last 8 bits of the base.
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

struct gdt32_descriptor_struct
{
    uint16_t size;
    uint32_t address;
} __attribute__((packed));
typedef struct gdt32_descriptor_struct gdt32_descriptor_t;

// ==============================================================================
// IDT
// https://wiki.osdev.org/Interrupt_Descriptor_Table

struct idt_type_attr_struct
{
    uint8_t gate_type : 4;
    uint8_t storage_segment : 1;
    uint8_t dpl : 2;
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

#endif // JOS_KERNEL_DT_H