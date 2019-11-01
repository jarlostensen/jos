#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stddef.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);
int sprintf (char * __restrict, const char * __restrict, ... );
int snprintf ( char * __restrict, size_t n, const char * __restrict, ... );

#ifdef __cplusplus
}
#endif

#endif
