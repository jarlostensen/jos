

#include "../kernel/kernel_detail.h"
#include <kernel/tasks.h>
#include <kernel/clock.h>
#include <multiboot.h>
#include <stdio.h>
#include <string.h>


void _k_modules_root_task(void* obj)
{
    multiboot_info_t* mboot = (multiboot_info_t*)obj;

    // ================================================================    
    _k_enable_interrupts();

    printf("\n-------------- root task, multiboot info @ 0x%x\n", mboot);
    
    const size_t kSize = 5000;
    void* mem = k_mem_alloc(kSize);
    memset(mem, 0xff, kSize);
    printf("test: allocated %d bytes @ 0x%x\n", kSize, (int)mem);
    
    uint32_t ms = k_clock_ms_since_boot();
    printf("waiting for a second starting at %d...", ms);    
    while(ms<=1000)
    {
        ms = k_clock_ms_since_boot();
        k_pause();
    }    
    printf("ok, we're at %dms\n", ms);
    
    uint64_t cpu_freq = _k_clock_est_cpu_freq();
    printf("CPU clocked to %lld MHz\n", cpu_freq/1000000);

    const uint64_t ms_to_wait = 2;
    printf("rdtsc timer wait for %lld @ %lld...", ms_to_wait, k_clock_ms_since_boot(), __rdtsc());
    uint64_t rdtsc_start = __rdtsc();
    uint64_t rdtsc_end = __rdtsc() + (cpu_freq * ms_to_wait + 500)/1000;
    while(__rdtsc() <= rdtsc_end)
        k_pause();
    
    printf("now = %lld, delta rdtsc = %lld", k_clock_ms_since_boot(), __rdtsc()-rdtsc_start);

    //k_mem_free(mem);

    JOS_KTRACE("halting\n");
    k_panic();
}
