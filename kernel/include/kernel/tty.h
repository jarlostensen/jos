#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <stdint.h>

void k_tty_initialize(void);
void k_tty_set_colour(uint8_t col);
void k_tty_putchar(char c);
void k_tty_write(const char* data, size_t size);
void k_tty_writestring(const char* data);
void k_tty_scroll_up();
void k_tty_set_column(size_t x);
void k_tty_set_row(size_t y);
size_t k_tty_curr_column();
size_t k_tty_curr_row();
void k_tty_disable_cursor();

#endif
