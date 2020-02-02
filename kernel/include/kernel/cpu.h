#ifndef _JOS_CPU_H
#define _JOS_CPU_H

#include <stdbool.h>
#include <cpuid.h>

typedef enum cpu_feature_enum
{
    kCpuFeature_x87 = 0x1,
    kCpuFeature_Vme = 0x2,
    kCpuFeature_DebuggingExtensions = 0x4,
    kCpuFeature_PageSizeExtension = 0x8,
    kCpuFeature_TSC = 0x10,
    kCpuFeature_MSR = 0x20,
    kCpuFeature_PAE = 0x40,
    kCpuFeature_APIC = 0x100,
    kCpuFeature_SEP = 0x400,
    kCpuFeature_ACPI = 0x200000,
} cpu_feature_t;

void k_cpu_init();

bool k_cpu_feature_present(cpu_feature_t feature);

// returns the highest extended function number supported by this CPU
unsigned int k_cpuid_max_extended();

// read MSR
inline void rdmsr(uint32_t msr, uint32_t* lo, uint32_t* hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}
// write MSR
inline void wrmsr(uint32_t msr, uint32_t lo, uint32_t hi)
{
   asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

#endif // _JOS_CPU_H