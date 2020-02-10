#include "../../kernel/kernel_detail.h"
#include "vga.h"

#include <string.h>

#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static const size_t VGA_PITCH = 80*2;

#ifdef _JOS_KERNEL_BUILD
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xb8000;
#else
#include <stdio.h>
// allocated separately
static uint16_t* VGA_MEMORY = 0;
#endif

void vga_display_size(int* width, int *height)
{
	*width = VGA_WIDTH;
	*height = VGA_HEIGHT;
}

void vga_blt(void* src_, size_t start_line, size_t stride, size_t width, size_t lines)
{
    _JOS_ASSERT(start_line < VGA_HEIGHT); 
    _JOS_ASSERT((stride & 1)==0);
    stride >>= 1;
	const int output_width = 2*min(VGA_WIDTH, width);
	lines = min(lines, VGA_HEIGHT - start_line);
	uint16_t* src = (uint16_t*)src_;
    uint16_t* dst = VGA_MEMORY + start_line*VGA_PITCH;
	while(lines--)
	{
        memcpy(dst, src, output_width);
		src += stride;
        dst += VGA_WIDTH;
	}
}

void vga_clear(void)
{
	vga_clear_to_val(vga_entry(' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK)));
}

void vga_clear_to_val(uint16_t entry)
{
	const uint32_t val = (uint32_t)entry | ((uint32_t)entry << 16);
    const size_t quad_pitch = (VGA_PITCH * VGA_HEIGHT)/4;
    uint32_t* wp = (uint32_t*)VGA_MEMORY;
    for(size_t p = 0; p < quad_pitch; ++p)
    {
        *wp++ = val;
    }	
}

void vga_init(void)
{
#ifndef _JOS_KERNEL_BUILD
	VGA_MEMORY = (uint16_t*)malloc(VGA_PITCH*VGA_HEIGHT);
#endif
    vga_clear();
}

#ifndef _JOS_KERNEL_BUILD
void vga_draw(void)
{
	uint16_t* vga_ptr = VGA_MEMORY;
	for(int y = 0; y < VGA_HEIGHT; ++y)
	{
		for(int x = 0; x < VGA_WIDTH; ++x)
		{
			printf("%c", (char)*vga_ptr++);
		}
		printf("\n");
	}
}
#endif
