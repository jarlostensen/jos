#pragma once

#ifndef _JOS_KERNEL_OUTPUT_CONSOLE_H
#define _JOS_KERNEL_OUTPUT_CONSOLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../jos.h"

// TEMPORARY

#define _TTY_HEIGHT 8
#define _TTY_WIDTH 25

typedef struct _console_output_driver
{
	// assumes stride in bytes
	void (*_blt)(void* src, size_t start_line, size_t stride, size_t width, size_t lines);
	void (*_clear)(uint16_t value);
} console_output_driver_t;

_JOS_PRIVATE_FUNC void _printf_driver_blt(void * src_, size_t start_line, size_t stride, size_t width, size_t lines)
{
#ifndef _JOS_KERNEL_BUILD
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
#else
	//TODO:
#endif
}

typedef struct _output_console
{
	uint16_t		_rows;
	uint16_t		_columns;
	int16_t			_start_row;
	int16_t			_col;
	int16_t			_row;
	//NOTE: buffer includes attribute byte
	uint16_t*		_buffer;

	console_output_driver_t* _driver;

} output_console_t;

//ZZZ:
extern output_console_t _stdout;

inline void output_console_create(output_console_t* con)
{
	_JOS_ASSERT(con);
	_JOS_ASSERT(con->_rows);
	_JOS_ASSERT(con->_columns);
	const int buffer_byte_size = con->_rows*con->_columns*sizeof(uint16_t);
	con->_buffer = (uint16_t*)malloc(buffer_byte_size);
	con->_start_row = 0;
	con->_col = con->_row = 0;
	memset(con->_buffer, '.', buffer_byte_size);
}

inline void output_console_destroy(output_console_t* con)
{
	if(!con || !con->_rows || !con->_columns || !con->_buffer)
		return;

	free(con->_buffer);
	memset(con,0,sizeof(output_console_t));
}

inline void output_console_flush(output_console_t* con)
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
		int lines_to_flush = min(lines_in_buffer, _TTY_HEIGHT);
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

inline void output_console_println(output_console_t* con)
{
	_JOS_ASSERT(con);
	_JOS_ASSERT(con->_buffer);
	con->_row = (con->_row+1) % con->_rows;
	if(con->_row == con->_start_row)
	{
		con->_start_row = (con->_start_row+1) % con->_rows;
	}
	con->_col = 0;
	//ZZZ:
	memset(con->_buffer+(con->_row*con->_columns), '.', con->_columns * sizeof(uint16_t));
}

inline void output_console_print(output_console_t* con, const char* line, char attr)
{
	_JOS_ASSERT(con);
	_JOS_ASSERT(con->_buffer);

	//TODO: horisontal scroll
	const int output_width = min(strlen(line), con->_columns);
	uint16_t cc = con->_row*con->_columns + con->_col;
	const uint16_t attr_mask = ((uint16_t)attr << 8);
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

#endif // _JOS_KERNEL_OUTPUT_CONSOLE_H