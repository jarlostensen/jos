#include <stdio.h>
#include "../libc_internal.h"

int _JOS_LIBC_FUNC_NAME(puts)(const char* string) {
	return printf("%s\n", string);
}
