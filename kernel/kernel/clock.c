
#include <stdint.h>
#include "../include/kernel/kernel.h"
#include "interrupts.h"
#include "kernel_detail.h"
#include "clock.h"

#include <stdio.h>

#define CLOCK_COMMAND 0x43
#define CLOCK_DATA 0x40

static void clock_irq_handler(int i)
{
    printf("clock (%d)!\n",i);
}

void k_init_clock()
{
    k_set_irq_handler(0,clock_irq_handler);

    // initialise PIT
    k_outb(CLOCK_COMMAND, 0x36);

    // set frequency 
    // TESTING @ 50 Hz
    uint32_t div = 1193180 / 1;

    // start the clock
    k_outb(CLOCK_DATA, div & 0xff);
    k_outb(CLOCK_DATA, (div>>8) & 0xff);

    k_enable_irq(0);
}