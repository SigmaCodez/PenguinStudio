/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <stdbool.h>
#include <stdint.h>

static inline uint64_t v4l2_capture_timeout(uint64_t configured_usec, uint64_t elapsed_usec, bool received_frame)
{
	const uint64_t startup_usec = 2000000;
	if (!received_frame && elapsed_usec < startup_usec) {
		const uint64_t remaining = startup_usec - elapsed_usec;
		return remaining > configured_usec ? remaining : configured_usec;
	}
	return configured_usec;
}
