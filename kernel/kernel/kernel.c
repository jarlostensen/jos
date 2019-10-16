#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../include/kernel/kernel.h"
#include "../include/kernel/tty.h"
#include "kernel_detail.h"
#include "dt.h"

#include "../arch/i386/vga.h"
#include "interrupts.h"
#include "clock.h"

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

void k_panic()
{
    terminal_set_colour(vga_entry_color(VGA_COLOR_WHITE,VGA_COLOR_LIGHT_BLUE));
    printf("\nKERNEL PANIC!");
    _k_halt_cpu();
}

static void isr_3_handler(uint16_t cs, uint32_t eip)
{
    printf("\tint 3 handler: next instruction is at 0x%x:0x%x\n",cs,eip);
}

static void irq_1_handler(int irq)
{
    printf("\tIRQ %d handler, keyboard\n", irq);
}

void _k_init(void *mboot)
{       
    terminal_initialize();
    terminal_disable_cursor();    
    _k_alloc_init();
    printf("_k_init, loading at 0x%x\n", mboot);
}

void _k_main()
{    
    printf("\n_kmain %s\n", k_is_protected_mode() ? "protected mode":"real mode");    

    _k_init_isrs();    
    k_set_isr_handler(3, isr_3_handler);
    k_set_irq_handler(1, irq_1_handler);
    _k_load_isrs();
    k_enable_irq(1);
    _k_enable_interrupts();
    k_init_clock();
    
    uint32_t ms = k_get_ms_since_boot();
    printf("waiting for a second starting at %d...", ms);
    while(ms<1000)
    {
        ms = k_get_ms_since_boot();
    }
    printf("ok, we're at %d\n", ms);

    k_panic();
}