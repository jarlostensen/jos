
#include "kernel_detail.h"
#include "cpu.h"
#include <stdio.h>
#include <string.h>

// Intel IA dev guide, 19.1.1

//NOTE: if these are clear in REAL mode it's a 286 but...that's not something we check for yet
#define CPU_FLAGS_8086      (0x7<<12)

// max supported basic and extended
static unsigned int _max_basic_cpuid = 0;
static unsigned int _max_extended_cpuid = 0;
static char _vendor_string[32] = {0};

void k_cpu_init()
{    
    JOS_KTRACE("k_cpu_init\n");
    unsigned int flags = k_eflags();
    _max_basic_cpuid = __get_cpuid_max(0, NULL);
    if(_max_basic_cpuid)
    {
        _max_extended_cpuid = __get_cpuid_max(0x80000000, NULL);
        JOS_KTRACE("_max_basic_cpu = %d, _max_extended_cpu = %x\n", _max_basic_cpuid, _max_extended_cpuid);

        //TODO: throw a fit if max basic or extended are too low...

        if ( k_cpu_feature_present(kCpuFeature_TSC | kCpuFeature_MSR) )
            JOS_KTRACE("cpu supports TSC & MSR\n");
        if( k_cpu_feature_present(kCpuFeature_APIC))
            JOS_KTRACE("cpu supports APIC\n");
        
        unsigned int _eax = 0, _ebx, _ecx, _edx;
        __get_cpuid(0, &_eax, &_ebx, &_ecx, &_edx);
        memcpy(_vendor_string + 0, &_ebx, sizeof(_ebx));
        memcpy(_vendor_string + 4, &_edx, sizeof(_edx));
        memcpy(_vendor_string + 8, &_ecx, sizeof(_ecx));
        _vendor_string[12] = 0;
        JOS_KTRACE("%s\n", _vendor_string);
    }
    else
    {
        JOS_KTRACE(" CPUID not supported\n");
        if(flags && CPU_FLAGS_8086 == CPU_FLAGS_8086)
        {
            JOS_KTRACE("unsupported CPU\n");
            printf("CPU is 8086, sorry...can't run.\n");
            k_panic();
        }
    }
    
}

bool k_cpu_feature_present(cpu_feature_t feature)
{
    uint32_t a,b,c,d;
    __cpuid(0x1, a,b,c,d);
    return (d & feature)==feature;
}