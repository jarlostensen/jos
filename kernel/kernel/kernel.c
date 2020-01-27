#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "kernel_detail.h"
#include "dt.h"
#include <multiboot.h>

#include "../arch/i386/vga.h"
#include "../arch/i386/hde/hde32.h"
#include "interrupts.h"
#include "memory.h"
#include "serial.h"
#include <kernel/clock.h>
#include <kernel/cpu.h>
#include <kernel/tasks.h>
#include <kernel/atomic.h>

// =======================================================

gdt_entry_t _k_gdt[] = {
    // null
    { .limit_low = 0, .base_low = 0, .base_middle = 0, .access.byte = 0, .granularity.byte = 0, .base_high = 0 },
    // kernel code MUST BE _JOS_KERNEL_CS_SELECTOR
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.byte = 0b10011010, .granularity.fields = { .limit_high = 0xf, .size = 1, .granularity = 1 }, .base_high = 0 },
    // kernel data
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.byte = 0b10010010, .granularity.byte = 0b11001111, .base_high = 0 },
    // user code
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.fields = { .rw = 1,  .executable = 1, .one=1, .privilege=3, .present = 1 }, .granularity.fields = { .limit_high = 0xf, .size = 1, .granularity = 1 }, .base_high = 0 },
    // user data
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.fields = { .rw = 1, .one=1, .privilege=3, .present = 1 }, .granularity.byte = 0b11001111, .base_high = 0 },
};
gdt32_descriptor_t _k_gdt_desc = {.size = sizeof(_k_gdt), .address = (uint32_t)(_k_gdt)};

// the 16 bit GDT is used for switching to real mode during boot (and perhaps later)
// Notably: 64K limit, 1 byte granularity, and 16 bit.
gdt_entry_t _k_gdt16[] = {
    // null
    { .limit_low = 0, .base_low = 0, .base_middle = 0, .access.byte = 0, .granularity.byte = 0, .base_high = 0 },
    // 16 bit code
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.fields = { .rw = 1,  .executable = 1, .one=1, .privilege=0, .present = 1 }, .granularity.fields = { .limit_high = 0, .size = 0, .granularity = 0 }, .base_high = 0 },
    // 16 bit data
    { .limit_low = 0xffff, .base_low = 0, .base_middle = 0, .access.fields = { .rw = 1,  .executable = 0, .one=1, .privilege=0, .present = 1 }, .granularity.fields = { .limit_high = 0, .size = 0, .granularity = 0 }, .base_high = 0 },
};
gdt32_descriptor_t _k_gdt16_desc = {.size = sizeof(_k_gdt16), .address = (uint32_t)(_k_gdt16)};

__attribute__((__noreturn__)) void k_panic(void)
{
    k_tty_set_colour(vga_entry_color(VGA_COLOR_WHITE,VGA_COLOR_LIGHT_BLUE));
    printf("\nKERNEL PANIC!");
    _k_halt_cpu();
    __builtin_unreachable();
}

static atomic_int_t _interrupts_enabled;
void _k_disable_interrupts(void)
{
    int expected = 1;
    if ( __atomic_compare_exchange_n(&_interrupts_enabled._val, &expected, 0, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) )
    {
        asm volatile ("cli");
    }
}

void _k_enable_interrupts(void)
{
    int expected = 0;
    if ( __atomic_compare_exchange_n(&_interrupts_enabled._val, &expected, 1, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) )
    {
        asm volatile ("sti");
    }
}

// ======================================================================================
// general isr/trap/fault handlers

extern void _k_mem_page_fault_handler(uint32_t error_code, uint16_t cs, uint32_t eip);

static void isr_0_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    (void)error_code;
    // divide by 0
    printf("\tdivide by zero @ 0x%x:0x%x\n",cs,eip);
}

static void isr_1_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    (void)error_code;
    // debug (hardware bps, instruction fetch bp, etc.)
    printf("\tdebug interrupt @ 0x%x:0x%x\n",cs,eip);
}

static void isr_2_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    (void)error_code;
    // divide by 0
    printf("\tdivide by zero @ 0x%x:0x%x\n",cs,eip);
}

static void isr_3_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    (void)error_code;
    printf("\tint 3 @ 0x%x:0x%x\n",cs,eip);
}

static void isr_4_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    (void)error_code;
    printf("\toverflow interrupt @ 0x%x:0x%x\n",cs,eip);
}

static void isr_5_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    (void)error_code;
    printf("\tbound range exceeded interrupt @ 0x%x:0x%x\n",cs,eip);
}

static void isr_6_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    (void)error_code;
    printf("\tinvalid opcode @ 0x%x:0x%x\n",cs,eip);
    _JOS_BOCHS_DBGBREAK();
}

static void irq_1_handler(int irq)
{
    printf("\tIRQ %d handler, keyboard\n", irq);
}

// in modules/modules.c
extern void _k_modules_root_task(void* obj);

void _k_main(uint32_t magic, multiboot_info_t *mboot)
{    
    // ================================================================
    // this section expects INTERRUPTS TO BE DISABLED
    // ================================================================

    k_tty_initialize();
    k_tty_disable_cursor();    
    k_serial_init();
   
    printf("=============================================\n");
    printf("This is the jOS kernel\n\n");

    _JOS_KTRACE("_k_init\n");
    if(magic!=MULTIBOOT_BOOTLOADER_MAGIC)
    {
        _JOS_KTRACE("error: not loaded with multiboot!\n");
        k_panic();
    }
            
    k_mem_init(mboot);
    k_cpu_init();            

    _k_init_isrs();    
    k_set_irq_handler(1, irq_1_handler);

    k_set_isr_handler(0, isr_0_handler);
    k_set_isr_handler(1, isr_1_handler);
    k_set_isr_handler(2, isr_2_handler);
    k_set_isr_handler(3, isr_3_handler);
    k_set_isr_handler(4, isr_4_handler);
    k_set_isr_handler(5, isr_5_handler);
    k_set_isr_handler(6, isr_6_handler);
    k_set_isr_handler(14, _k_mem_page_fault_handler);
    
    _k_load_isrs();

    k_enable_irq(1);
    k_clock_init();

    // initialise tasks and hand over to the root task (this call never returns)
    k_tasks_init(_k_modules_root_task, (void*)mboot);
}