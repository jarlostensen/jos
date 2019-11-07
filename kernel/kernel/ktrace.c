
#include <stdio.h>
#include <string.h>
#include "kernel_detail.h"
#include "serial.h"
#include "clock.h"

void _k_trace(const char* format,...)
{
    if(!format || !format[0])
        return;
    char buffer[256];
    va_list parameters;
    va_start(parameters, format);
    int written = snprintf(buffer, sizeof(buffer), "[%lld] ", k_get_ms_since_boot());
    written += vsnprintf(buffer+written, sizeof(buffer)-written, format, parameters);
    va_end(parameters);
    k_serial_write(kCom1, buffer, written);
}