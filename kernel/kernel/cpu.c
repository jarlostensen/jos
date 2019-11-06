
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

static void _smp_init()
{    
    static const uint32_t kMpTag = 0x5F504d54; // _MP_
    
    //TODO: this is always 16 byte aligned so we can skip through a bit faster
    int scan(uint32_t start, const uint32_t end)
    {
        uint32_t* rp = (uint32_t*)start;
        while(rp<=(const uint32_t*)end)
        {
            if(*rp == kMpTag)
            {
                JOS_KTRACE("_MP_ tag found @ 0x%x\n", rp);
                return 1;
            }
            rp = (uint32_t*)((uint8_t*)rp + 0x10);
        }
        return 0;
    }

    // look in regions [0x400, 0x500>, [0x80000,0xa0000>, [0xf0000,0x100000> for "_MP_" tag
    if(scan(0x400,0x500) || scan(0x80000,0xa0000) || scan(0xf0000,0xfffff))
    {        
        //TODO: found MP tag, parse structures
    }
    else
    {    
        JOS_KTRACE("no _MP_ tag found, processor appears to be single core\n");    
    }
}

void k_cpu_init()
{    
    JOS_KTRACE("k_cpu_init\n");
    unsigned int flags = k_eflags();
    _max_basic_cpuid = __get_cpuid_max(0, NULL);
    if(_max_basic_cpuid)
    {
        unsigned int _eax = 0, _ebx, _ecx, _edx;
        int cpu_ok = 0;
        _max_extended_cpuid = __get_cpuid_max(0x80000000, NULL);
        if( _max_extended_cpuid>=0x80000001)
        {
            __get_cpuid(0x80000001,&_eax, &_ebx, &_ecx, &_edx);
            // supports 64 bit mode?
            cpu_ok = (_edx & (1<<29)) == (1<<29);

            // added bonus, supports 1gig pages?
            if( (_edx & (1<<26)) == (1<<26))
            {
                JOS_KTRACE("cpu supports 1Gig pages\n");
            }
        }

        if(!cpu_ok)
        {
            JOS_KTRACE("error: this CPU does not support the functionality we need (max extended CPUID is 0x%x)\n", _max_extended_cpuid);    
            k_panic();        
        }

        JOS_KTRACE("_max_basic_cpu = %d, _max_extended_cpu = %x\n", _max_basic_cpuid, _max_extended_cpuid);        
        __get_cpuid(0, &_eax, &_ebx, &_ecx, &_edx);
        memcpy(_vendor_string + 0, &_ebx, sizeof(_ebx));
        memcpy(_vendor_string + 4, &_edx, sizeof(_edx));
        memcpy(_vendor_string + 8, &_ecx, sizeof(_ecx));
        _vendor_string[12] = 0;
        JOS_KTRACE("%s\n", _vendor_string);

        if ( k_cpu_feature_present(kCpuFeature_TSC | kCpuFeature_MSR) )
            JOS_KTRACE("cpu supports TSC & MSR\n");
        if( k_cpu_feature_present(kCpuFeature_APIC))
        {
            JOS_KTRACE("cpu supports APIC\n");
            _smp_init();
        }
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