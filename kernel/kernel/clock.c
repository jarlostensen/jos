
#include <stdint.h>
#include "../include/kernel/kernel.h"
#include "interrupts.h"
#include "kernel_detail.h"
#include "clock.h"

#include <stdio.h>

#define CLOCK_COMMAND 0x43
#define CLOCK_DATA 0x40

#define HZ              2000
#define CLOCK_FREQ      1193182
#define ONE_FP32        0x100000000
#define HALF_FP32       0x010000000

uint32_t _k_clock_freq_frac = 0;
volatile uint32_t _k_clock_frac = 0;
volatile uint64_t _k_ms_elapsed;
// kernel.asm
extern void _k_update_clock();

static void clock_irq_handler(int i)
{
    (void)i;
    _k_update_clock();    
    //TODO: anythin' else...?
}

uint64_t k_get_ms_since_boot()
{
    return _k_ms_elapsed;
}

void k_init_clock()
{
    double ddiv = (double)CLOCK_FREQ/(double)HZ;
    ddiv = (ddiv+0.5);
    uint16_t div16 = (uint16_t)(ddiv);
    ddiv *= (double)ONE_FP32;
    _k_clock_freq_frac = (uint32_t)(((ddiv * 1000.0)/(double)CLOCK_FREQ));
    _k_ms_elapsed = 0;

    printf("k_init_clock: starting PIT with divider 0x%x, frac in 32.32fp is 0x%x...", (int)div16, _k_clock_freq_frac);

    // run once to initialise counters
    _k_update_clock();
    
    // initialise PIT
    k_outb(CLOCK_COMMAND, 0x36);
    // set frequency         
    k_outb(CLOCK_DATA, div16 & 0xff);
    k_outb(CLOCK_DATA, (div16>>8) & 0xff);

    // start the clock IRQ
    k_set_irq_handler(0,clock_irq_handler);
    k_enable_irq(0);

    printf("ok\n");
}