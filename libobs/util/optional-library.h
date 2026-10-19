// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Optional Linux capabilities must not report an absent top-level runtime as
// an application error. Missing dependencies, invalid ELF files and symbol
// resolution errors are still errors. Use exact SONAMEs (no suffix expansion).
#ifdef __linux__
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include "base.h"

static inline void *os_dlopen_optional(const char *soname)
{
	void *library = dlopen(soname, RTLD_NOW);
	if (!library) {
		const char *error = dlerror();
		const size_t length = strlen(soname);
		// Both dlerror and strerror are localized. Match the requested object
		// and the localized ENOENT suffix, not an English-only error sentence.
		const char *not_found = strerror(ENOENT);
		const size_t suffix_length = strlen(not_found);
		const size_t error_length = error ? strlen(error) : 0;
		const bool absent = error && error_length >= length + suffix_length + 4 &&
			strncmp(error, soname, length) == 0 && strncmp(error + length, ": ", 2) == 0 &&
			strncmp(error + error_length - suffix_length - 2, ": ", 2) == 0 &&
			strcmp(error + error_length - suffix_length, not_found) == 0;
		blog(absent ? LOG_INFO : LOG_ERROR, "Optional runtime %s: %s", soname,
		     error ? error : "unknown loader error");
	}
	return library;
}
#endif
