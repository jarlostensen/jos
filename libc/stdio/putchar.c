#include <stdio.h>

#if defined(__is_libk)
#include <kernel/tty.h>
#endif

int putchar(int ic) {
#if defined(__is_libk)
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
#else
	// TODO: Implement stdio and the write system call.
#endif
	return ic;
}
