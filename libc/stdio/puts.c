#include <stdio.h>
#include "../libc_internal.h"

int _JOS_LIBC_FUNC_NAME(puts)(const char* string) {
	return _JOS_LIBC_FUNC_NAME(printf)("%s\n", string);
}
