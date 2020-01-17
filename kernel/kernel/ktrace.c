
#include <stdio.h>
#include <string.h>
#include "kernel_detail.h"
#include "serial.h"
#include <kernel/clock.h>

void _k_trace_buf(const void* data, size_t length)
{
    if(!data || !length)
        return;
    char buffer[256];    
    int written = snprintf(buffer, sizeof(buffer), "[%lld] ", k_clock_ms_since_boot());
    length = length < (sizeof(buffer)-written-2) ? length:(sizeof(buffer)-written-2);
    memcpy(buffer+written, data, length);
    buffer[written+length] ='\n';
    buffer[written+length+1] =0;
    k_serial_write(kCom1, buffer, written+length+1);
}

void _k_trace(const char* format,...)
{
    if(!format || !format[0])
        return;
    char buffer[256];
    va_list parameters;
    va_start(parameters, format);
    int written = snprintf(buffer, sizeof(buffer), "[%lld] ", k_clock_ms_since_boot());
    written += vsnprintf(buffer+written, sizeof(buffer)-written, format, parameters);
    va_end(parameters);
    k_serial_write(kCom1, buffer, written);
}