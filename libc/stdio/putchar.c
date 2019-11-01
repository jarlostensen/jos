#include <stdio.h>

#include <kernel/tty.h>

int putchar(int ic) {
	char c = (char) ic;
	if(c=='\t')
	{
		static const char kTabs[4] = {' ',' ', ' ', ' '};
		k_tty_write(kTabs,sizeof(kTabs));
	}
	else
	{
		k_tty_write(&c, sizeof(c));
	}
	return ic;
}
