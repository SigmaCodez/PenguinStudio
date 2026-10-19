// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <locale.h>
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
	const char *locales[] = {"C", "de_DE.UTF-8", "de_CH.UTF-8"};
	for (size_t locale = 0; locale < sizeof(locales) / sizeof(locales[0]); ++locale) {
		if (!setlocale(LC_ALL, locales[locale]))
			continue;
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
		printf("PASS (%s): absent runtime, valid library, invalid ELF, missing dependency\n", locales[locale]);
	}
	return 0;
}
