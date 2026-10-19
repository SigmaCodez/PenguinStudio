// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../libobs/util/optional-library.h"

static int last_level;
void blog(int level, const char *format, ...)
{
	(void)format;
	last_level = level;
}

int main(int argc, char **argv)
{
	if (argc != 5)
		return 2;
	for (int i = 1; i < argc; ++i) {
		last_level = 0;
		void *handle = os_dlopen_optional(argv[i]);
		const int expected = i == 1 ? LOG_INFO : i == 2 ? 0 : LOG_ERROR;
		if (last_level != expected || (handle != NULL) != (i == 2)) {
			fprintf(stderr, "Unexpected loader result for case %d: level %d\n", i, last_level);
			return 1;
		}
		if (handle)
			dlclose(handle);
	}
	puts("PASS: absent runtime, valid library, invalid ELF, missing dependency");
	return 0;
}
