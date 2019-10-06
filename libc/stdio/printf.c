#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

static int printdecimal(long d)
{
	int written = 0;	
    if (d < 0)
    {
        putchar((int)'-');
        d *= -1;
		++written;
    }
	if(d < 10)
	{
		putchar((int)'0'+(int)d);
		return 1;
	}

    // simple and dumb but it works...
    long  pow_10 = 1;
    long dd = d;

    // find highest power of 10 
    while (dd > 9)
    {
        dd /= 10;
        pow_10 *= 10;
    }

    // print digits from MSD to LSD
    do {
        putchar((int)'0' + (int)dd);
		++written;
        // modulo
        d = d - (dd * pow_10);
        if (!d)
            break;
        dd = d;
        while (dd > 9)
            dd /= 10;
        pow_10 /= 10;
    } while (pow_10 > 1);
    if(d)
	{
        putchar((int)'0' + (int)dd);
		++written;
	}
    else
	{
        // trailing zeros
        while (pow_10 > 1)
        {
            putchar((int)'0');
            pow_10 /= 10;
			++written;
        }
	}
	return written;
}

static int printhex(long d)
{
    static const char* kHexDigits = "0123456789abcdef";
    int written = 0;
    if (d < 0)
    {
        d *= -1;
        putchar((int)'-');
        written = 1;
    }
    if (d <= 256)
    {
        if(d > 16)
            putchar((int)kHexDigits[(d&0xf0)>>4]);
        putchar((int)kHexDigits[(d & 0xf)]);
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
        putchar((int)kHexDigits[hi]);
        putchar((int)kHexDigits[lo]);
        written += 2;
        --high_idx;
        --bytes;
    } while (high_idx>=0);

    return written;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

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
			if (!print(format, amount))
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
			if (!print(&c, sizeof(c)))
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
			if (!print(str, len))
				return -1;
			written += len;
		}
		else if (*format == 'd' || *format == 'l') {
			format++;
			long d;
			if(format[0]=='d')
				d = (long) va_arg(parameters, int);
			else
				d = (long) va_arg(parameters, long);
			if(!maxrem)
				return -1;
			
			written += printdecimal(d);
		}
		else if (*format == 'x') {
			format++;
			long d;
			d = (long) va_arg(parameters, int);
			if(!maxrem)
				return -1;
			
			written += printhex(d);
		}
		 else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}
