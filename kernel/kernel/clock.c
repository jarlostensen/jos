
#include <stdint.h>
#include "kernel_detail.h"
#include "interrupts.h"
#include "clock.h"
#include "cpu.h"

#include <stdio.h>


//NOTE: setting this to the same as what the Linux kernel (currently) uses...
#define HZ              200
#define CLOCK_FREQ      1193182
#define ONE_FP32        0x100000000
#define HALF_FP32       0x010000000

// http://www.brokenthorn.com/Resources/OSDev16.html

#define PIT_COMMAND 0x43
#define PIT_DATA_0  0x40
#define PIT_DATA_2  0x42

// PIT command word flags
#define PIT_COUNTER_0           0
#define PIT_COUNTER_1           0x20
#define PIT_COUNTER_2           0x40

#define PIT_MODE_SQUAREWAVE     0x06     // binary, square wave
#define PIT_RL_DATA             0x30    // LSB, then MSB
#define PIT_MODE_ONESHOT        0x01

uint32_t _k_clock_freq_frac = 0;
uint32_t _k_clock_freq_whole = 0;
//TODO: use atomic
volatile uint32_t _k_clock_frac = 0;
volatile uint32_t _k_clock_whole = 0;
volatile uint64_t _k_ms_elapsed;
// kernel.asm
extern void _k_update_clock();

// See
// https://github.com/torvalds/linux/blob/master/arch/x86/kernel/tsc.c

static void clock_irq_handler(int i)
{
    (void)i;
    _k_update_clock();    
    
    //TODO: check if any outstanding timers are pending etc.
}

uint64_t k_get_ms_since_boot()
{
    return _k_ms_elapsed;
}


uint64_t k_clock_get_ms_res()
{
    return (uint64_t)_k_clock_freq_whole<<32 | (uint64_t)_k_clock_freq_frac;
}

void k_wait_oneshot_one_period()
{
    k_outb(PIT_COMMAND, PIT_COUNTER_2 | PIT_MODE_ONESHOT);
    k_outb(PIT_DATA_2, 0xff);
    k_outb(PIT_DATA_2, 0xff);

    //TODO: dig deeper, this disables the speaker, does that free up channel 2?
    k_outb(0x61,(k_inb(0x61) & ~0x02) | 0x01);

    // dummy read, give time for the next edge rise
    k_inb(PIT_DATA_2);
    char msb = k_inb(PIT_DATA_2);
    do {
        k_inb(PIT_DATA_2);
        msb = k_inb(PIT_DATA_2);
    } while(msb);
}

uint64_t _k_clock_est_cpu_freq()
{
    // not sure what the right number should be?
    int tries = 3;
    //NOTE: this is assuming the 18 Hz standard clock used
    const uint64_t elapsed_ms = 55;
    uint64_t min_cpu_hz = ~(uint64_t)0;
    uint64_t max_cpu_hz = 0;
    do
    {
        uint64_t rdtsc_start = __rdtsc();
        k_wait_oneshot_one_period();
        uint64_t rdtsc_end = __rdtsc();
        const uint64_t cpu_hz = 1000*(rdtsc_end - rdtsc_start)/elapsed_ms;
        if(cpu_hz < min_cpu_hz)
            min_cpu_hz = cpu_hz;
        if(cpu_hz > max_cpu_hz)
            max_cpu_hz = cpu_hz;
    } while(tries--);

    return (max_cpu_hz + min_cpu_hz)/2;
}

void k_clock_init()
{
    // clock divisor
    double ddiv = (double)CLOCK_FREQ/(double)HZ;
    // round up 
    ddiv = (ddiv+0.5);
    // divisor for PIT
    uint16_t div16 = (uint16_t)(ddiv);
    // scale to 32.32 fixed point
    ddiv *= (double)ONE_FP32;
    // milliseconds per tick
    uint64_t scaled = (((ddiv * 1000.0)/(double)CLOCK_FREQ));
    // whole and fractional part
    _k_clock_freq_whole = (uint32_t)(scaled >> 32);
    _k_clock_freq_frac = (uint32_t)(scaled & 0x00000000ffffffff);
    _k_ms_elapsed = 0;

    JOS_KTRACE("k_init_clock: starting PIT for %d HZ with divider %d, resolution is %d.%d\n", HZ, (int)div16, _k_clock_freq_whole, _k_clock_freq_frac);

    // run once to initialise counters
    _k_update_clock();
    
    // initialise PIT
    k_outb(PIT_COMMAND, PIT_COUNTER_0 | PIT_MODE_SQUAREWAVE | PIT_RL_DATA);
    // set frequency         
    k_outb(PIT_DATA_0, div16 & 0xff);
    k_outb(PIT_DATA_0, (div16>>8) & 0xff);

    // start the clock IRQ
    k_set_irq_handler(0,clock_irq_handler);
    k_enable_irq(0);
}