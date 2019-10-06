#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <stdint.h>

void terminal_initialize(void);
void terminal_set_colour(uint8_t col);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_scroll_up();
void terminal_set_column(size_t x);
void terminal_set_row(size_t y);
size_t terminal_curr_column();
size_t terminal_curr_row();
void terminal_disable_cursor();

#endif
