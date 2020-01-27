#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../libc_internal.h"

typedef int (*print_func_t)(void* ctx, const char* data, size_t len);
typedef int (*putchar_func_t)(void* ctx, int character);

static bool _is_printable(int c)
{
    // printavle ASCII characters (including delete)
    // [32,127]
    return c > 31 && c < 128;
}

// the various printf/sprintf etc. functions use an implementation template (_vprint_impl), this structure 
// effectively provides the different policies used.
struct _printf_ctx_struct
{
    print_func_t    _print;
    putchar_func_t  _putchar;
    void*           _that;
};
typedef struct _printf_ctx_struct _printf_ctx_t;

static int printdecimal(_printf_ctx_t* ctx, long long d, int un_signed)
{
    int written = 0;
    if (!d)
    {
        ctx->_putchar(ctx->_that, (int)'0');
        return 1;
    }

    if (d < 0)
    {
        if (!un_signed)
        {
            ctx->_putchar(ctx->_that, (int)'-');
            ++written;
        }
        d *= -1;
    }
    // simple and dumb but it works...
    int pow_10 = 1;
    long long dd = d;

    // find highest power of 10 
    while (dd > 9)
    {
        dd /= 10;
        pow_10 *= 10;
    }

    // print digits from MSD to LSD
    while (true) {
        ctx->_putchar(ctx->_that, (int)'0' + (int)dd);
        ++written;
        d = d - (dd * pow_10);
        pow_10 /= 10;
        if (!pow_10)
            break;
        dd = d / pow_10;
    };
    return written;
}

static int printhex(_printf_ctx_t* ctx, long long d)
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
        if (d > 15)
            ctx->_putchar(ctx->_that, (int)kHexDigits[(d & 0xf0) >> 4]);
        ctx->_putchar(ctx->_that, (int)kHexDigits[(d & 0xf)]);
        return 1;
    }
    int high_idx = 0;
    long long dd = d;
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
        size_t hi = (*bytes & 0xf0) >> 4;
        ctx->_putchar(ctx->_that, (int)kHexDigits[hi]);
        ctx->_putchar(ctx->_that, (int)kHexDigits[lo]);
        written += 2;
        --high_idx;
        --bytes;
    } while (high_idx >= 0);

    return written;
}

static int printbin(_printf_ctx_t* ctx, unsigned long long d)
{
    int written = 0;
    unsigned long long dd = d;
    unsigned long long bc = 0;
    while (dd) 
    {
        dd >>= 1;
        ++bc;
    }
    while (bc)
    {
        unsigned long dc = d & (1ull << (bc-1));
        dc >>= (bc-1);
        ctx->_putchar(ctx->_that, (int)'0' + (int)dc);
        --bc;
        ++written;
    }
    return written;
}

int _vprint_impl(_printf_ctx_t* ctx, const char* __restrict format, va_list parameters)
{
    int written = 0;

    while (*format != '\0') {
        size_t maxrem = INT_MAX - written;
        if (!maxrem) {
            // TODO: Set errno to EOVERFLOW.
            return -1;
        }

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
        }

        const char* format_begun_at = format++;

        //TODO: parsing, but ignoring
        char flag = 0;
        if (format[0] == '+' || format[0] == '-' || format[0] == ' ' || format[0] == '0' || format[0] == '#')
        {
            flag = *format++;
        }
        (void)flag;
        
        //TODO: parsing, but ignoring
        int width = 0;
        int pow10 = 1;
        while (format[0] >= '0' && format[0] <= '9')
        {
            width += (*format - '0') * pow10;
            pow10 *= 10;
            ++format;
        }
        //TODO: parsing, but ignoring
        int precision = 0;
        if (format[0] == '.')
        {
            ++format;
            pow10 = 1;
            while (format[0] >= '0' && format[0] <= '9')
            {
                precision += (*format - '0') * pow10;
                pow10 *= 10;
                ++format;
            }
        }

        //TODO: parsed, but only l is handled
        char length[2] = { 0,0 };
        if (format[0] == 'h' || format[0] == 'l' || format[0] == 'L' || format[0] == 'z' || format[0] == 'j' || format[0] == 't')
        {
            length[0] = *format++;
            if (format[0] == 'h' || format[0] == 'l')
                length[1] = *format++;
        }

        char type = *format++;

        switch (type)
        {
        case 'c':
        {
            char c = (char)va_arg(parameters, int);
            if (ctx->_print(ctx->_that, &c, sizeof(c))==EOF)
                return EOF;
            written++;
        }
        break;
        case 's':
        {
            const char* str = va_arg(parameters, const char*);
            size_t len = strlen(str);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return EOF;
            }
            if(len)
            {
                if (ctx->_print(ctx->_that, str, len)==EOF)
                    return EOF;
                written += len;
            }
        }
        break;
        case 'd':
        case 'i':
        {
            if (length[0] == 'l')
            {
                if (length[1] == 'l')
                {
                    long long d = va_arg(parameters, long long);
                    written += printdecimal(ctx, d, 0);
                }
                else
                {
                    long d = va_arg(parameters, long);
                    written += printdecimal(ctx, d, 0);
                }
            }
            else
            {
                int d = va_arg(parameters, int);
                written += printdecimal(ctx, d, 0);
            }
        }
        break;
        case 'u':
        {
            if (length[0] == 'l')
            {
                if (length[1] == 'l')
                {
                    unsigned long long d = va_arg(parameters, unsigned long long);
                    written += printdecimal(ctx, d, 1);
                }
                else
                {
                    unsigned long d = va_arg(parameters, unsigned long);
                    written += printdecimal(ctx, d, 1);
                }
            }
            else
            {
                unsigned int d = va_arg(parameters, unsigned int);
                written += printdecimal(ctx, d, 1);
            }
        }
        break;
        case 'x':
        {
            if (length[0] == 'l')
            {
                if (length[1] == 'l')
                {
                    unsigned long long d = va_arg(parameters, unsigned long long);
                    written += printhex(ctx, d);
                }
                else
                {
                    unsigned long d = va_arg(parameters, unsigned long);
                    written += printhex(ctx, d);
                }
            }
            else
            {
                unsigned int d = va_arg(parameters, unsigned int);
                written += printhex(ctx, d);
            }
        }
        break;
        case 'b':
        {
            if (length[0] == 'l')
            {
                if (length[1] == 'l')
                {
                    unsigned long long d = va_arg(parameters, unsigned long long);
                    written += printbin(ctx, d);
                }
                else
                {
                    unsigned long d = va_arg(parameters, unsigned long);
                    written += printbin(ctx, d);
                }
            }
            else
            {
                unsigned int d = va_arg(parameters, unsigned int);
                written += printbin(ctx, d);
            }
        }
        break;
        case 'f':
        {
            //TODO: print float of width.precision
            float f = (float)va_arg(parameters, double);
            written += printdecimal(ctx, (long long)f, 0);
        }
        break;
        default:
        {
            format = format_begun_at;
            size_t len = strlen(format);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return EOF;
            }
            if (ctx->_print(ctx->_that, format, len)==EOF)
                return EOF;
            written += len;
            format += len;
        }
        break;
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
            return EOF;
	}
	return (int)length;
}

static int console_putchar(void* ctx, int c) {
    (void)ctx;
    if(!_is_printable(c))
        return EOF;
    return putchar(c);
}

int _JOS_LIBC_FUNC_NAME(printf)(const char* __restrict format, ...) 
{
	va_list parameters;
	va_start(parameters, format);
    int count = _vprint_impl(&(_printf_ctx_t){ ._print = console_print, ._putchar = console_putchar }, format, parameters);
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
    if(!_is_printable(c))
        return EOF;
    buffer_t* ctx = (buffer_t*)ctx_;    
    *ctx->_wp++ = (char)c;
    return c;
}

static int buffer_print(void *ctx_, const char* data, size_t length) 
{
    const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
	{
		if (buffer_putchar(ctx_, bytes[i]) == EOF)
			return EOF;
	}
	return (int)length;
}

int _JOS_LIBC_FUNC_NAME(sprintf)(char * __restrict buffer, const char * __restrict format, ... )
{
    if(!buffer || !format || !format[0])
    {
        return 0;
    }

	va_list parameters;
	va_start(parameters, format);
    int written = _vprint_impl(&(_printf_ctx_t) { 
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
    if(!_is_printable(c))
        return EOF;
    buffer_t* ctx = (buffer_t*)ctx_;
    if(ctx->_wp != ctx->_end)
        *ctx->_wp++ = (char)c;
    return 1;
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
			return EOF;
	}
	return (int)length;
}

int _JOS_LIBC_FUNC_NAME(snprintf) ( char * buffer, size_t n, const char * format, ... )
{
    if(!buffer || !n || !format || !format[0])
        return 0;

    va_list parameters;
	va_start(parameters, format);
    int written = _vprint_impl(&(_printf_ctx_t) {
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

int _JOS_LIBC_FUNC_NAME(vsnprintf)(char* buffer, size_t n, const char* format, va_list parameters)
{
    if(!buffer || !n || !format || !format[0])
        return 0;

    int written = _vprint_impl(&(_printf_ctx_t) {
        ._print = buffer_n_print,
            ._putchar = buffer_n_putchar,
            ._that = (void*) & (buffer_t) { ._wp = buffer, ._end = buffer + (n - 1) }
    },
        format, parameters);
    buffer[written < (int)n ? written : (int)n - 1] = 0;
    return written;
}
