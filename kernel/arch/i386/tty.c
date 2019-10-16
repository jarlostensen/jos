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

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

void terminal_initialize(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;
	const uint16_t entry = vga_entry(' ', terminal_color) ;
	size_t index = 0;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			terminal_buffer[index++] = entry;
		}
	}
}

void terminal_disable_cursor()
{
	// https://wiki.osdev.org/VGA_Hardware#Port_0x3C4.2C_0x3CE.2C_0x3D4
	k_outb(0x3d4, 0x0a);
	k_outb(0x3d5, 0x20);
}

void terminal_set_colour(uint8_t col)
{
	terminal_color = col;
}

void terminal_scroll_up() {
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
	const uint16_t entry = vga_entry(' ', terminal_color) ;
	for(size_t j = 0; j < VGA_WIDTH>>1; ++j) 
	{		
		wp[j] = (entry | entry << 16);
	}
}

void terminal_set_column(size_t x)
{
	if(x < VGA_WIDTH)
		terminal_column = x;
	else
		terminal_column = VGA_WIDTH-1;
}

void terminal_set_row(size_t y)
{
	if(y < VGA_HEIGHT)
		terminal_row = y;
	else
		terminal_row = VGA_HEIGHT-1;
}

size_t terminal_curr_column()
{
	return terminal_column;
}

size_t terminal_curr_row()
{
	return terminal_row;
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
	unsigned char uc = c;

	if(uc == '\n' || uc == '\r') {
		if( terminal_row >= (VGA_HEIGHT-1))
		{
			terminal_scroll_up();
		}
		terminal_set_row(terminal_row+1);
		terminal_set_column(0);
		return;
	}

	terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}
