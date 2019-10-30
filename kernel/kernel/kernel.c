#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../include/kernel/kernel.h"
#include "../include/kernel/tty.h"
#include "kernel_detail.h"
#include "dt.h"
#include "multiboot.h"

#include "../arch/i386/vga.h"
#include "interrupts.h"
#include "clock.h"
#include "cpu.h"
#include "paging.h"

// =======================================================

gdt_entry_t _k_gdt[] = {
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

void k_panic()
{
    k_tty_set_colour(vga_entry_color(VGA_COLOR_WHITE,VGA_COLOR_LIGHT_BLUE));
    printf("\nKERNEL PANIC!");
    _k_halt_cpu();
}

static void isr_3_handler(uint32_t error_code, uint16_t cs, uint32_t eip)
{
    (void)error_code;
    printf("\tint 3 handler: next instruction is at 0x%x:0x%x\n",cs,eip);
}

static void irq_1_handler(int irq)
{
    printf("\tIRQ %d handler, keyboard\n", irq);
}

void _k_init(uint32_t magic, multiboot_info_t *mboot)
{       
    k_tty_initialize();
    k_tty_disable_cursor();    

    printf("_k_init...");
    if(magic!=MULTIBOOT_BOOTLOADER_MAGIC)
    {
        printf("FAIL, not loaded with multiboot!\n");
        k_panic();
    }
    else
    {
        printf("multiboot detected\n");
        if(mboot->flags & MULTIBOOT_INFO_MEMORY)
        {
            printf("\tmem_lower = %d KB, mem_upper = %d KB\n", mboot->mem_lower, mboot->mem_upper);
            bool has_available = false;
            for (multiboot_memory_map_t* mmap = (multiboot_memory_map_t *) mboot->mmap_addr; 
                (unsigned long) mmap <mboot->mmap_addr + mboot->mmap_length; 
                mmap = (multiboot_memory_map_t *) ((unsigned long) mmap + mmap->size + sizeof (mmap->size)))
            {
                if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
                {
                    printf ("\tavailable 0x%x bytes @ 0x%x\n",
                            (unsigned) mmap->len,
                            (unsigned) mmap->addr);
                    has_available = true;
                }                
            }
            if(!has_available)
            {
                printf("\nERROR: no available RAM\n");
                k_panic();
            }
        }
    }
    printf("ok\n");
    _k_alloc_init();
    k_init_cpu();    
}

void _k_main()
{    
    printf("\n_kmain %s\n", k_is_protected_mode() ? "protected mode":"real mode");    

    _k_init_isrs();    
    k_set_isr_handler(3, isr_3_handler);
    k_set_irq_handler(1, irq_1_handler);
    _k_load_isrs();
    k_paging_init(); 

    k_enable_irq(1);
    _k_enable_interrupts();
    k_init_clock();

    uint32_t ms = k_get_ms_since_boot();
    printf("waiting for a second starting at %d...", ms);
    while(ms<1000)
    {
        ms = k_get_ms_since_boot();
    }
    printf("ok, we're at %dms\n", ms);
    
    ms = k_get_ms_since_boot();
    printf("waiting for one period starting at %d...", ms);
    uint32_t ms_start = k_get_ms_since_boot();
    k_wait_oneshot_one_period();
    ms = k_get_ms_since_boot();
    printf("done, one 1/18 period took ~%d ms\n", ms-ms_start);

    k_panic();
}