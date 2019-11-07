
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

// multi processor floating pointer structure
// found in memory by scanning (see _smp_init)
typedef struct smp_fp_struct
{
    uint32_t    _sig;
    uint32_t    _phys_address;
    uint8_t     _length;
    uint8_t     _specrev;
    uint8_t     _checksum;
    uint8_t     _feature_information;
    uint8_t     _feature_information_2:7;
    uint8_t     _imcr_present:1;
    uint8_t     _reserved[3];
} __attribute__((packed)) smp_fp_t;

static smp_fp_t *_smp_fp = 0;

static void _smp_init()
{    
    static const uint32_t kMpTag = (('_'<<24) | ('P'<<16) | ('M'<<8) | '_');
    
    int scan(uint32_t start, const uint32_t length)
    {        
        const uint32_t* end = (const uint32_t*)(start + length);
        uint32_t* rp = (uint32_t*)start;
        JOS_KTRACE("SMP scan 0x%x -> 0x%x\n", rp, end);
        while(rp<=end)
        {
            if(*rp == kMpTag)
            {                
                _smp_fp = (smp_fp_t*)rp;
                if(_smp_fp->_length==1 && (_smp_fp->_specrev == 0x1 || _smp_fp->_specrev == 0x4))
                {
                    JOS_KTRACE("MPF structure @ 0x%x, version 1.%d\n", rp, _smp_fp->_specrev);
                    return 1;
                }
                JOS_KTRACE("found invalid MPF structure @ 0x%x\n", rp);
                // die?
            }
            // skip MP structure size (16 bytes)
            rp = (uint32_t*)((uint8_t*)rp + 0x10);
        }        
        return 0;
    }

    // look for SMP signature in candiate RAM regions
    if(scan(0,0x400) || scan(639 * 0x400, 0x400) || scan(0xf0000,0x10000))
    {        
        //TODO: found MP tag, parse structures
    }
    else
    {    
        JOS_KTRACE("no valid _MP_ tag found, processor appears to be single core\n");    
    }
}

unsigned int k_cpuid_max_extended()
{
    return _max_extended_cpuid;
}

void k_cpu_init()
{    
    JOS_KTRACE("k_cpu_init\n");
    unsigned int flags = k_eflags();
    if((flags & CPU_FLAGS_8086) == CPU_FLAGS_8086)
    {
        JOS_KTRACE("error: unsupported CPU (8086)\n");
        printf("CPU is 8086, sorry...can't run.\n");
        k_panic();
    }

    _max_basic_cpuid = __get_cpuid_max(0, NULL);
    if(!_max_basic_cpuid)
    {
        JOS_KTRACE("error: CPUID not supported\n");
        k_panic();
    }

    unsigned int _eax = 0, _ebx, _ecx, _edx;
    int cpu_ok = 0;
    _max_extended_cpuid = __get_cpuid_max(0x80000000, NULL);
    if( _max_extended_cpuid>=0x80000001)
    {
        __get_cpuid(0x80000001,&_eax, &_ebx, &_ecx, &_edx);
        // supports 64 bit long mode?
        cpu_ok = (_edx & (1<<29)) == (1<<29);
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
    //ZZZ: something is strange about this here trace; it appears to generate garbage for a couple of subsequent traces...investigate!
    JOS_KTRACE("%s\n", _vendor_string);

    if ( k_cpu_feature_present(kCpuFeature_TSC | kCpuFeature_MSR) )
        JOS_KTRACE("TSC & MSR supported\n");
    if( k_cpu_feature_present(kCpuFeature_APIC))
    {
        JOS_KTRACE("APIC present\n");
        _smp_init();
    }

    int pae = 0;
    __get_cpuid(0x1, &_eax, &_ebx, &_ecx, &_edx);
    if( (_edx & (1<<6)) == (1<<6))
    {
        JOS_KTRACE("PAE supported\n");
        pae = 1;
    }
    if( (_edx & (1<<17)) == (1<<17))
    {
        JOS_KTRACE("PSE-36 supported\n");
    }

    if(k_cpuid_max_extended() >= 0x80000008)
    {
        __get_cpuid(0x80000008, &_eax, &_ebx, &_ecx, &_edx);
        JOS_KTRACE("cpu physical address size %d bits\n", _eax & 0xff);
        JOS_KTRACE("cpu linear address size %d bits\n", (_eax >> 8) & 0xff);
    }
    else
    {
        if(pae)
            JOS_KTRACE("cpu physical address size 36 bits\n");
        else
            JOS_KTRACE("cpu physical address size 32 bits\n");
        JOS_KTRACE("cpu linear address size 32 bits\n");
    }    
}

bool k_cpu_feature_present(cpu_feature_t feature)
{
    uint32_t a,b,c,d;
    __cpuid(0x1, a,b,c,d);
    return (d & feature)==feature;
}