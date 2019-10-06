#include <string.h>
#include <stdint.h>

void* memset(void* bufptr, int value, size_t size) {
	uint32_t* buf = (uint32_t*) bufptr;
	size_t rem = size & 3;
	size >>= 2;
	uint32_t v32 = (uint8_t)value;
	v32 |= (v32 << 8);
	v32 |= (v32 << 16);
	for (size_t i = 0; i < size; i++)
		*buf++ = v32;
	unsigned char* cbuf = (unsigned char*)buf;
	while(rem--)
		*cbuf++ = (unsigned char)value;
	return bufptr;
}
