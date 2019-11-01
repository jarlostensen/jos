
#include "kernel_detail.h"
#include <kernel/kernel.h>
#include "cpu.h"
#include <stdio.h>

// Intel IA dev guide, 19.1.1

//NOTE: if these are clear in REAL mode it's a 286 but...that's not something we check for yet
#define CPU_FLAGS_8086      (0x7<<12)

// max supported basic and extended
static unsigned int _max_basic_cpuid = 0;
static unsigned int _max_extended_cpuid = 0;
//TODO:static int _has_fpu = 0;

void k_cpu_init()
{    
    printf("k_cpu_init");
    unsigned int flags = k_eflags();
    _max_basic_cpuid = __get_cpuid_max(0, NULL);
    if(_max_basic_cpuid)
    {
        _max_extended_cpuid = __get_cpuid_max(0x80000000, NULL);
        printf(" _max_basic_cpu = %d, _max_extended_cpu = %x\n", _max_basic_cpuid, _max_extended_cpuid);
    }
    else
    {
        printf(" CPUID not supported ");
        if(flags && CPU_FLAGS_8086 == CPU_FLAGS_8086)
        {
            printf("CPU is 8086, sorry...can't run.\n");
            k_panic();
        }
    }
    
}