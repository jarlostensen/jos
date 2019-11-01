#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef int (*print_func_t)(void* ctx, const char* data, size_t len);
typedef int (*putchar_func_t)(void* ctx, int character);

// the various printf/sprintf etc. functions use an implementation template (_printf_impl), this structure 
// effectively provides the different policies used.
struct _printf_ctx_struct
{
    print_func_t    _print;
    putchar_func_t  _putchar;
    void*           _that;
};
typedef struct _printf_ctx_struct _printf_ctx_t;

static int printdecimal(_printf_ctx_t* ctx, long d)
{
	int written = 0;
    if (d < 0)
    {
        ctx->_putchar(ctx->_that, (int)'-');
        d *= -1;
        ++written;
    }
    // simple and dumb but it works...
    int pow_10 = 1;
    int dd = d;

    // find highest power of 10 
    while (dd > 9)
    {
        dd /= 10;
        pow_10 *= 10;
    }

    // print digits from MSD to LSD
    while(true) {
        ctx->_putchar(ctx->_that, (int)'0' + dd);
        ++written;
        d = d - (dd * pow_10);
        pow_10 /= 10;
        if (!pow_10)
            break;
        dd = d / pow_10;
    };
    return written;
}

static int printhex(_printf_ctx_t* ctx, long d)
{
    static const char* kHexDigits = "0123456789abcdef";
    int written = 0;
    if (d < 0)
    {
        d *= -1;
		//don't write a sign         
    }
    if (d <= 256)
    {
        if(d > 15)
            ctx->_putchar(ctx->_that, (int)kHexDigits[(d&0xf0)>>4]);
        ctx->_putchar(ctx->_that, (int)kHexDigits[(d & 0xf)]);
        return 1;
    }    
    int high_idx = 0;
    long dd = d;
    while (dd > 15)
    {
        dd >>= 4;
        ++high_idx;
    }
    // convert to byte offset
    high_idx >>= 1;
	// read from MSD to LSD
    const char* bytes = (const char*)(&d) + high_idx;    
    do
    {
		//NOTE: this will always "pad" the output to an even number of nybbles
        size_t lo = *bytes & 0x0f;
        size_t hi = (*bytes & 0xf0)>>4;
        ctx->_putchar(ctx->_that, (int)kHexDigits[hi]);
        ctx->_putchar(ctx->_that, (int)kHexDigits[lo]);
        written += 2;
        --high_idx;
        --bytes;
    } while (high_idx>=0);

    return written;
}

// https://en.wikipedia.org/wiki/Printf_format_string
int _printf_impl(_printf_ctx_t* ctx, const char * __restrict format, va_list parameters)
{	
	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!ctx->_print(ctx->_that, format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!ctx->_print(ctx->_that, &c, sizeof(c)))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!ctx->_print(ctx->_that, str, len))
				return -1;
			written += len;
		}
		else if (format[0] == 'd' || format[0] == 'i' || format[0] == 'l') 
        {
            // [l[l]]d
            long d;
            if (format[0] == 'd' || format[0] == 'i')
            {
                d = (long)va_arg(parameters, int);
                ++format;
            }
            else
            {
                ++format;
                if (format[0] == 'l')
                {
                    //zzz:
                    d = (long)va_arg(parameters, long long);
                    ++format;
                }
                else
                {
                    d = (long)va_arg(parameters, long);
                }
                // strictly correct but the form %l[l] is often used without d or i
                if (format[0] == 'd' || format[0] == 'i')
                    ++format;
            }

            if (!maxrem)
                return -1;

            written += printdecimal(ctx, d);
        }
		else if (*format == 'x') {
			format++;
			long d;
			d = (long) va_arg(parameters, int);
			if(!maxrem)
				return -1;
			
			written += printhex(ctx, d);
		}
		 else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!ctx->_print(ctx->_that, format, len))
				return -1;
			written += len;
			format += len;
		}
	}
	return written;
}

// ================================================================================================================

static int console_print(void* ctx, const char* data, size_t length) {
    (void)ctx;
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
	{
		if (putchar(bytes[i]) == EOF)
			return false;
	}
	return true;
}

static int console_putchar(void* ctx, int c) {
    (void)ctx;
    return putchar(c) != EOF;
}

int printf(const char* restrict format, ...) 
{
	va_list parameters;
	va_start(parameters, format);
    int count = _printf_impl(&(_printf_ctx_t){ ._print = console_print, ._putchar = console_putchar }, format, parameters);
    va_end(parameters);
    return count;
}

// ================================================================================================================

typedef struct buffer_ctx_struct 
{
    char*       _wp;
    const char* _end;
} buffer_t;

static int buffer_putchar(void* ctx_, int c)
{
    buffer_t* ctx = (buffer_t*)ctx_;
    *ctx->_wp++ = (char)c;
    return true;
}

static int buffer_print(void *ctx_, const char* data, size_t length) 
{
    const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
	{
		if (buffer_putchar(ctx_, bytes[i]) == EOF)
			return false;
	}
	return true;
}

int sprintf(char * __restrict buffer, const char * __restrict format, ... )
{
	va_list parameters;
	va_start(parameters, format);
    int written = _printf_impl(&(_printf_ctx_t) { 
        ._print = buffer_print, 
        ._putchar = buffer_putchar, 
        ._that = (void*)&(buffer_t){ ._wp = buffer } 
        }, 
        format, parameters); 
    buffer[written] = 0;
    va_end(parameters);
	return written;
}

static int buffer_n_putchar(void* ctx_, int c)
{
    buffer_t* ctx = (buffer_t*)ctx_;
    if(ctx->_wp != ctx->_end)
        *ctx->_wp++ = (char)c;    
    return true;
}

static int buffer_n_print(void *ctx_, const char* data, size_t length) 
{
    buffer_t* ctx = (buffer_t*)ctx_;
    const unsigned char* bytes = (const unsigned char*) data;
    size_t rem_bytes = (size_t)ctx->_end - (size_t)ctx->_wp;
    length = length < rem_bytes ? length : rem_bytes;
	for (size_t i = 0; i < length; i++)
	{
		if (buffer_n_putchar(ctx_, bytes[i]) == EOF)
			return false;
	}
	return true;
}

int snprintf ( char * buffer, size_t n, const char * format, ... )
{
    va_list parameters;
	va_start(parameters, format);
    int written = _printf_impl(&(_printf_ctx_t) {
        ._print = buffer_n_print,
            ._putchar = buffer_n_putchar,
            ._that = (void*) & (buffer_t) { ._wp = buffer, ._end = buffer+(n-1) }
    },
        format, parameters);
    written = written < (int)n ? written : (int)n-1;
    buffer[written] = 0;
    va_end(parameters);
	return written;
}