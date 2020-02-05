#include <kernel/output_console.h>
#ifdef _JOS_KERNEL_BUILD
#include "../arch/i386/vga.h"
#else
_JOS_PRIVATE_FUNC void _printf_driver_blt(void * src_, size_t start_line, size_t stride, size_t width, size_t lines)
{
	stride >>= 1;
	const int output_width = min(_TTY_WIDTH, width);
	lines = min(lines, _TTY_HEIGHT - start_line);
	uint16_t* src = (uint16_t*)src_;
	while(lines--)
	{
		for(int c = 0; c < output_width; ++c)
		{
			printf("%c", (char)src[c]);
		}
		if ( lines )
			printf("\n");
		src += stride;
	}
}
#endif

output_console_t _stdout;
static console_output_driver_t _vga_driver;

void output_console_init(void)
{
    vga_init();
    _vga_driver._blt = vga_blt;	
    _vga_driver._clear = vga_clear_to_val;

	_stdout._columns = 100;    
	_stdout._rows = _TTY_HEIGHT*2;
	_stdout._driver = &_vga_driver;
    _stdout._attr = vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK);
	output_console_create(&_stdout);
}

void output_console_set_fg(output_console_t* con, uint8_t fgcol)
{
    con->_attr &= 0xf0;
    con->_attr |= fgcol;
}

void output_console_set_bg(output_console_t* con, uint8_t bgcol)
{
    con->_attr &= 0x0f;
    con->_attr |= (bgcol << 4);
}

void output_console_flush(output_console_t* con)
{
	_JOS_ASSERT(con);
	_JOS_ASSERT(con->_buffer);
	const int output_width = min(con->_columns, _TTY_WIDTH);
	const size_t stride = (size_t)con->_columns*2;

	if(con->_start_row < con->_row)
	{
		int lines_to_flush = min(con->_row+1, _TTY_HEIGHT);
		const int flush_start_row = con->_row - min(_TTY_HEIGHT-1, con->_row-con->_start_row);
		//NOTE: always flush from col 0
		con->_driver->_blt(con->_buffer + flush_start_row * con->_columns, 0, stride, (size_t)output_width, lines_to_flush);
	}
	else
	{
		const int lines_in_buffer = con->_row + (con->_rows - con->_start_row) + 1; //< +1 to convert _row to count
		int flush_row = con->_rows - (lines_in_buffer - con->_row - 1); //< as above
		//NOTE: we always flush from col 0
		uint16_t cc = flush_row * con->_columns;		
		const size_t lines = con->_rows - flush_row;
		con->_driver->_blt(con->_buffer + cc, 0, stride, (size_t)output_width, lines);		
#ifndef _JOS_KERNEL_BUILD
	printf("\n");
#endif
		// next batch from the top
		con->_driver->_blt(con->_buffer, lines, stride, (size_t)output_width, con->_row + 1);
	}
}

inline void output_console_print(output_console_t* con, const char* line)
{
	_JOS_ASSERT(con);
	_JOS_ASSERT(con->_buffer);
	//TODO: horisontal scroll
	const int output_width = min(strlen(line), con->_columns);
	uint16_t cc = con->_row*con->_columns + con->_col;
	const uint16_t attr_mask = ((uint16_t)con->_attr << 8);
	for(int c = 0; c < output_width; ++c)
	{
		if(line[c]=='\n')
		{
			output_console_println(con);
			cc = con->_row*con->_columns;
		}
		else 
		{
			con->_buffer[cc++] = attr_mask | line[c];		
			++con->_col;
		}
	}
}