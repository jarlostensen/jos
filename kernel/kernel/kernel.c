#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../include/kernel/kernel.h"
#include "../include/kernel/tty.h"
#include "kernel_detail.h"
#include "dt.h"

#include "../arch/i386/vga.h"
#include "interrupts.h"

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

static void isr_3_handler(void)
{
    printf("\tint 3 handler!\n");
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

    k_init_isrs();    
    k_set_isr_handler(3, isr_3_handler);
    k_load_isrs();
    _k_enable_interrupts();
    asm("int $3");
    
    k_panic();
}