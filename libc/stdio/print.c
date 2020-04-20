#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include <kernel/output_console.h>
#include "../libc_internal.h"

#ifndef _JOS_KERNEL_BUILD
#include <intrin.h>
#endif

typedef int (*print_func_t)(void* ctx, const char* data, size_t len);
typedef int (*putchar_func_t)(void* ctx, int character);
typedef void (*flush_func_t)(void* ctx);

// the various printf/sprintf etc. functions use an implementation template (_vprint_impl), this structure 
// effectively provides the different policies used.
typedef struct _ctx_struct
{
	print_func_t    _print;
	putchar_func_t  _putchar;
	flush_func_t    _flush;
	void* _that;
} ctx_t;

static int printdecimal(ctx_t* ctx, long long d, int un_signed)
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

static int printhex(ctx_t* ctx, int width, long long d)
{
	static const char* kHexDigits = "0123456789abcdef";
	int written = 0;

	if (d < 0)
	{
		d *= -1;
		//don't write a sign
	}
	if (width)
	{
		// calculate leading zeros required
		if(!d)
		{
			// all zeros; just output them
			written += width;
			while (width--)
			{
				ctx->_putchar(ctx->_that, '0');
			}
			return written;
		}

#ifdef _JOS_KERNEL_BUILD
		unsigned long d_width;
		(d_width = __builtin_clzll(d), ++d_width);
#else
		unsigned long d_width;
		(_BitScanReverse64(&d_width, d), ++d_width);
#endif
		d_width = (d_width / 4) + (d_width & 3 ? 1 : 0);
		if (width >= (int)d_width)
		{
			width -= d_width;			
		}
	}

	if (d <= 256)
	{
		// write leading 0's
		written += width;
		while (width--)
		{
			ctx->_putchar(ctx->_that, '0');
		}
		
		if (d > 15)
		{
			ctx->_putchar(ctx->_that, (int)kHexDigits[(d & 0xf0) >> 4]);
			++written;
		}
		ctx->_putchar(ctx->_that, (int)kHexDigits[(d & 0xf)]);
		return written+1;
	}

	// round padding down to nearest 2 since we always write pairs of digits below
	width &= ~1;
	written += width;
	while (width--)
	{
		ctx->_putchar(ctx->_that, '0');
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

static int printbin(ctx_t* ctx, unsigned long long d)
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
		unsigned long dc = d & (1ull << (bc - 1));
		dc >>= (bc - 1);
		ctx->_putchar(ctx->_that, (int)'0' + (int)dc);
		--bc;
		++written;
	}
	return written;
}

int _vprint_impl(ctx_t* ctx, const char* __restrict format, va_list parameters)
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

		if (format[0])
		{
			const char* format_begun_at = format++;

			//TODO: parsing, but ignoring
			char flag = 0;
			if (format[0] == '+' || format[0] == '-' || format[0] == ' ' || format[0] == '0' || format[0] == '#')
			{
				flag = *format++;
			}
			(void)flag;

			// width modifier
			int width = 0;			
			int width_digits = 0;
			int pow10 = 1;
			while (format[width_digits] >= '0' && format[width_digits] <= '9')
			{
				++width_digits;
				pow10 *= 10;
			}
			if(width_digits)
			{
				pow10 /= 10;				
				while (format[0] >= '0' && format[0] <= '9')
				{
					width += (*format - '0') * pow10;
					pow10 /= 10;
					++format;
				}
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
				ctx->_print(ctx->_that, &c, sizeof(c));
				written++;
			}
			break;
			case 's':
			{
				const char* str = va_arg(parameters, const char*);
				//NOTE: this is *not* standard, supporting a width modifier for %s
				size_t len = width ? (size_t)width : strlen(str);
				if (maxrem < len) {
					// TODO: Set errno to EOVERFLOW.
					return EOF;
				}
				if (len)
				{
					ctx->_print(ctx->_that, str, len);
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
						written += printhex(ctx, width, d);
					}
					else
					{
						unsigned long d = va_arg(parameters, unsigned long);
						written += printhex(ctx, width, d);
					}
				}
				else
				{
					unsigned int d = va_arg(parameters, unsigned int);
					written += printhex(ctx, width, d);
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
				ctx->_print(ctx->_that, format, len);
				written += len;
				format += len;
			}
			break;
			}
		}
	}

	if (ctx->_flush)
		ctx->_flush(ctx->_that);

	return written;
}

// ================================================================================================================

typedef struct _printf_ctx
{
#define _PRINTF_CTX_LINE_LENGTH 256
	char    _line[_PRINTF_CTX_LINE_LENGTH];
	size_t  _wp;
} printf_ctx_t;

static void console_flush(void* ctx)
{
	printf_ctx_t* printf_ctx = (printf_ctx_t*)ctx;
	if (printf_ctx->_wp)
	{
		output_console_print(&_stdout, printf_ctx->_line);
		output_console_flush(&_stdout);
		printf_ctx->_wp = 0;
	}
}

static int console_print(void* ctx, const char* data, size_t length_) {
	printf_ctx_t* printf_ctx = (printf_ctx_t*)ctx;
	size_t length = length_;
	while (length)
	{
		//NOTE: this can never be 0, we always flush when the buffer is full
		size_t sub_length = min(_PRINTF_CTX_LINE_LENGTH - printf_ctx->_wp, length);
		memcpy(printf_ctx->_line + printf_ctx->_wp, data, sub_length);
		length -= sub_length;
		printf_ctx->_wp += sub_length;
		if (printf_ctx->_wp == _PRINTF_CTX_LINE_LENGTH)
		{
			console_flush(ctx);
		}
	}

	return (int)length_;
}

static int console_putchar(void* ctx, int c) {
	printf_ctx_t* printf_ctx = (printf_ctx_t*)ctx;
	//NOTE: this can never be 0, we always flush when the buffer is full
	memcpy(printf_ctx->_line + printf_ctx->_wp, &c, sizeof(c));
	++printf_ctx->_wp;
	if (printf_ctx->_wp == _PRINTF_CTX_LINE_LENGTH)
	{
		console_flush(ctx);
	}
	return 1;
}

int _JOS_LIBC_FUNC_NAME(printf)(const char* __restrict format, ...)
{
	va_list parameters;
	va_start(parameters, format);
	int count = _vprint_impl(&(ctx_t) {
		._print = console_print,
			._putchar = console_putchar,
			._flush = console_flush,
			._that = (void*) & (printf_ctx_t) { ._wp = 0 }
	},
		format, parameters);
	va_end(parameters);
	return count;
}

int _JOS_LIBC_FUNC_NAME(puts)(const char* string) {
	return _JOS_LIBC_FUNC_NAME(printf)("%s\n", string);
}

// ================================================================================================================

typedef struct buffer_ctx_struct
{
	char* _wp;
	const char* _end;
} buffer_t;

static int buffer_putchar(void* ctx_, int c)
{
	buffer_t* ctx = (buffer_t*)ctx_;
	_JOS_ASSERT(ctx->_wp != ctx->_end);
	*ctx->_wp++ = (char)c;
	return (ctx->_wp != ctx->_end ? c : EOF);
}

static int buffer_print(void* ctx_, const char* data, size_t length)
{
	const unsigned char* bytes = (const unsigned char*)data;
	for (size_t i = 0; i < length; i++)
	{
		buffer_t* ctx = (buffer_t*)ctx_;
		_JOS_ASSERT(ctx->_wp != ctx->_end);
		*ctx->_wp++ = (char)bytes[i];
	}
	return (int)length;
}

int _JOS_LIBC_FUNC_NAME(sprintf_s)(char* __restrict buffer, size_t buffercount, const char* __restrict format, ...)
{
	if (!buffer || !format || !format[0])
	{
		return 0;
	}

	va_list parameters;
	va_start(parameters, format);
	int written = _vprint_impl(&(ctx_t) {
		._print = buffer_print,
			._putchar = buffer_putchar,
			._that = (void*) & (buffer_t) { ._wp = buffer, ._end = buffer + buffercount }
	},
		format, parameters);
	buffer[written] = 0;
	va_end(parameters);
	return written;
}

static int buffer_n_putchar(void* ctx_, int c)
{
	buffer_t* ctx = (buffer_t*)ctx_;
	if (ctx->_wp != ctx->_end)
		*ctx->_wp++ = (char)c;
	return 1;
}

static int buffer_n_print(void* ctx_, const char* data, size_t length)
{
	buffer_t* ctx = (buffer_t*)ctx_;
	const unsigned char* bytes = (const unsigned char*)data;
	size_t rem_bytes = (size_t)ctx->_end - (size_t)ctx->_wp;
	length = length < rem_bytes ? length : rem_bytes;
	for (size_t i = 0; i < length; i++)
	{
		if (buffer_n_putchar(ctx_, bytes[i]) == EOF)
			return EOF;
	}
	return (int)length;
}

int _JOS_LIBC_FUNC_NAME(snprintf) (char* buffer, size_t n, const char* format, ...)
{
	if (!buffer || !n || !format || !format[0])
		return 0;

	va_list parameters;
	va_start(parameters, format);
	int written = _vprint_impl(&(ctx_t) {
		._print = buffer_n_print,
			._putchar = buffer_n_putchar,
			._that = (void*) & (buffer_t) { ._wp = buffer, ._end = buffer + (n - 1) }
	},
		format, parameters);
	written = written < (int)n ? written : (int)n - 1;
	buffer[written] = 0;
	va_end(parameters);
	return written;
}

int _JOS_LIBC_FUNC_NAME(vsnprintf)(char* buffer, size_t n, const char* format, va_list parameters)
{
	if (!buffer || !n || !format || !format[0])
		return 0;

	int written = _vprint_impl(&(ctx_t) {
		._print = buffer_n_print,
			._putchar = buffer_n_putchar,
			._that = (void*) & (buffer_t) { ._wp = buffer, ._end = buffer + (n - 1) }
	},
		format, parameters);
	buffer[written < (int)n ? written : (int)n - 1] = 0;
	return written;
}
