#include <stdio.h>

#include "../libc_internal.h"

#ifdef _JOS_KERNEL_BUILD
#include <kernel/tty.h>
#endif

int _JOS_LIBC_FUNC_NAME(putchar)(int ic) {
	char c = (char) ic;
	if(c=='\t')
	{
		static const char kTabs[4] = {' ',' ', ' ', ' '};
#ifdef _JOS_KERNEL_BUILD
		k_tty_write(kTabs,sizeof(kTabs));
#else
		printf("\t");
#endif
	}
	else
	{
#ifdef _JOS_KERNEL_BUILD
		k_tty_write(&c, sizeof(c));
#else
		printf("%c",c);
#endif
	}
	return ic;
}
