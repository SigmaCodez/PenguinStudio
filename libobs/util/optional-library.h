// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Optional Linux capabilities must not report an absent top-level runtime as
// an application error. Missing dependencies, invalid ELF files and symbol
// resolution errors are still errors. Use exact SONAMEs (no suffix expansion).
#ifdef __linux__
#include <dlfcn.h>
#include <string.h>
#include "base.h"

static inline void *os_dlopen_optional(const char *soname)
{
	void *library = dlopen(soname, RTLD_NOW);
	if (!library) {
		const char *error = dlerror();
		const size_t length = strlen(soname);
		const bool absent = error && strncmp(error, soname, length) == 0 &&
			strcmp(error + length, ": cannot open shared object file: No such file or directory") == 0;
		blog(absent ? LOG_INFO : LOG_ERROR, "Optional runtime %s: %s", soname,
		     error ? error : "unknown loader error");
	}
	return library;
}
#endif
