
#include <stdio.h>
#include <string.h>
#include "kernel_detail.h"
#include "serial.h"
#include "clock.h"

void _k_trace(const char* msg)
{
    if(!msg || !msg[0])
        return;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%lld] %s", k_get_ms_since_boot(), msg);
    k_serial_write(kCom1, buffer, strlen(buffer));
}