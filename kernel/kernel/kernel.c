#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "kernel_detail.h"
#include "dt.h"
#include "multiboot.h"

#include "../arch/i386/vga.h"
#include "../arch/i386/hde/hde32.h"
#include "interrupts.h"
#include "clock.h"
#include "cpu.h"
#include "paging.h"
#include "serial.h"

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

// ======================================================================================
// general isr/trap/fault handlers

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
}

static void irq_1_handler(int irq)
{
    printf("\tIRQ %d handler, keyboard\n", irq);
}

uint32_t _k_check_memory_availability(multiboot_info_t* mboot, uint32_t from_phys)
{
    /*
    * NOTE: this function MUST NOT use any variables not within its scope.
    * it is invoked prior to page mapping initialisation.
    */
    if(mboot->flags & MULTIBOOT_INFO_MEMORY)
    {
        for (multiboot_memory_map_t* mmap = (multiboot_memory_map_t *) mboot->mmap_addr; 
            (unsigned long) mmap <mboot->mmap_addr + mboot->mmap_length; 
            mmap = (multiboot_memory_map_t *) ((unsigned long) mmap + mmap->size + sizeof (mmap->size)))
        {
            if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
            {                
                if( from_phys > mmap->addr && from_phys < mmap->addr+mmap->len)
                {
                    return mmap->addr+mmap->len - from_phys;
                }
            }
        }
    }
    return 0;
}

void _k_main(uint32_t magic, multiboot_info_t *mboot)
{    
    k_tty_initialize();
    k_tty_disable_cursor();    
    k_serial_init();

    printf("=============================================\n");
    printf("This is the jOS kernel\n\n");

    JOS_KTRACE("_k_init\n");
    if(magic!=MULTIBOOT_BOOTLOADER_MAGIC)
    {
        JOS_KTRACE("error: not loaded with multiboot!\n");
        k_panic();
    }
    else
    {
        if(mboot->flags & MULTIBOOT_INFO_MEMORY)
        {
            JOS_KTRACE("mem_lower = %d KB, mem_upper = %d KB\n", mboot->mem_lower, mboot->mem_upper);
            bool has_available = false;
            for (multiboot_memory_map_t* mmap = (multiboot_memory_map_t *) mboot->mmap_addr; 
                (unsigned long) mmap <mboot->mmap_addr + mboot->mmap_length; 
                mmap = (multiboot_memory_map_t *) ((unsigned long) mmap + mmap->size + sizeof (mmap->size)))
            {
                if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
                {
                    JOS_KTRACE("available 0x%x bytes @ 0x%x\n",
                            (unsigned) mmap->len,
                            (unsigned) mmap->addr);
                    has_available = true;
                }
            }
            if(!has_available)
            {
                JOS_KTRACE("error: no available RAM\n");
                k_panic();
            }
        }
    }    
    //TESTING: alloc init requires more paging setup
    k_panic();

    k_alloc_init();
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
    _k_load_isrs();
    k_paging_init(); 

    k_enable_irq(1);
    k_clock_init();
    _k_enable_interrupts();
    
    uint32_t ms = k_get_ms_since_boot();
    printf("waiting for a second starting at %d...", ms);    
    while(ms<=1000)
    {
        ms = k_get_ms_since_boot();
        k_pause();
    }    
    printf("ok, we're at %dms\n", ms);
    
    uint64_t cpu_freq = _k_clock_est_cpu_freq();
    printf("CPU clocked to %lld MHz\n", cpu_freq/1000000);

    const uint64_t ms_to_wait = 2;
    printf("rdtsc timer wait for %lld @ %lld...", ms_to_wait, k_get_ms_since_boot());
    uint64_t rdtsc_end = __rdtsc() + (cpu_freq * ms_to_wait + 500)/1000;
    while(__rdtsc() <= rdtsc_end)
        k_pause();
    
    printf("now = %lld", k_get_ms_since_boot());

    JOS_KTRACE("halting\n");
    k_panic();
}