#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/kernel.h>
#include <kernel/tty.h>
#include "../../kernel/kernel_detail.h"

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static const size_t VGA_PITCH = 80*2;
static uint32_t VGA_BASE = 0xb8000;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xb8000;

static size_t _row;
static size_t _column;
static uint8_t _colour;
static uint16_t* _buffer;

void k_tty_initialize(void) {
	_row = 0;
	_column = 0;
	_colour = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	_buffer = VGA_MEMORY;
	const uint16_t entry = vga_entry(' ', _colour) ;
	size_t index = 0;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			_buffer[index++] = entry;
		}
	}
}

void k_tty_disable_cursor()
{
	// https://wiki.osdev.org/VGA_Hardware#Port_0x3C4.2C_0x3CE.2C_0x3D4
	k_outb(0x3d4, 0x0a);
	k_outb(0x3d5, 0x20);
}

void k_tty_set_colour(uint8_t col)
{
	_colour = col;
}

void k_tty_scroll_up() {
	uint32_t* wp = (uint32_t*)(VGA_BASE);
	const uint32_t * rp = (const uint32_t*)(VGA_BASE + VGA_PITCH);
	
	for(size_t i = 0; i < VGA_HEIGHT-1; ++i) 
	{
		memcpy(wp,rp,VGA_PITCH);
		wp += VGA_PITCH>>2;
		rp += VGA_PITCH>>2;
	}
	// clear last line
	wp = (uint32_t*)(VGA_MEMORY + VGA_WIDTH*(VGA_HEIGHT-1));
	const uint16_t entry = vga_entry(' ', _colour) ;
	for(size_t j = 0; j < VGA_WIDTH>>1; ++j) 
	{		
		wp[j] = (entry | entry << 16);
	}
}

void k_tty_set_column(size_t x)
{
	if(x < VGA_WIDTH)
		_column = x;
	else
		_column = VGA_WIDTH-1;
}

void k_tty_set_row(size_t y)
{
	if(y < VGA_HEIGHT)
		_row = y;
	else
		_row = VGA_HEIGHT-1;
}

size_t k_tty_curr_column()
{
	return _column;
}

size_t k_tty_curr_row()
{
	return _row;
}

void k_tty_setcolor(uint8_t color) {
	_colour = color;
}

void k_tty_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	_buffer[index] = vga_entry(c, color);
}

void k_tty_putchar(char c) {
	unsigned char uc = c;

	if(uc == '\n' || uc == '\r') {
		if( _row >= (VGA_HEIGHT-1))
		{
			k_tty_scroll_up();
		}
		k_tty_set_row(_row+1);
		k_tty_set_column(0);
		return;
	}

	k_tty_putentryat(uc, _colour, _column, _row);
	if (++_column == VGA_WIDTH) {
		_column = 0;
		if (++_row == VGA_HEIGHT)
			_row = 0;
	}
}

void k_tty_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		k_tty_putchar(data[i]);
}

void k_tty_writestring(const char* data) {
	k_tty_write(data, strlen(data));
}
