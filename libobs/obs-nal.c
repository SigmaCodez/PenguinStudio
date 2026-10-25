/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "obs-nal.h"
#include <string.h>

/* NOTE: I noticed that FFmpeg does some unusual special handling of certain
 * scenarios that I was unaware of, so instead of just searching for {0, 0, 1}
 * we'll just use the code from FFmpeg - http://www.ffmpeg.org/ */
static const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
	const size_t length = (size_t)(end - p);
	if (length < 3)
		return end;
	const uint8_t *last = end - 3;
	const size_t alignment = (4 - ((uintptr_t)p & 3)) & 3;
	const uint8_t *a = p + alignment;

	for (; p < a && p <= last; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	const uint8_t *word_end = length >= 6 ? end - 6 : p;
	for (; length >= 6 && p <= word_end; p += 4) {
		uint32_t x;
		memcpy(&x, p, sizeof(x));

		if ((x - 0x01010101) & (~x) & 0x80808080) {
			if (p[1] == 0) {
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p + 1;
			}

			if (p[3] == 0) {
				if (p[2] == 0 && p[4] == 1)
					return p + 2;
				if (p[4] == 0 && p[5] == 1)
					return p + 3;
			}
		}
	}

	for (; p <= last; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	return end;
}

const uint8_t *obs_nal_find_startcode(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *out = ff_avc_find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1])
		out--;
	return out;
}
