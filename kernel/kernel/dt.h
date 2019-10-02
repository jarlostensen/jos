#pragma once
#ifndef JOS_KERNEL_DT_H
#define JOS_KERNEL_DT_H
#include <stdint.h>

// ==============================================================================
// GDT
// https://wiki.osdev.org/Global_Descriptor_Table

struct gdt_access_entry_struct
{
    uint8_t accessed:1;
    uint8_t rw:1;
    uint8_t direction:1;
    uint8_t executable:1;
    uint8_t type:1;
    uint8_t privilege:2;
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