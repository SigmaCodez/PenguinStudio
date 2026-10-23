/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../plugins/linux-v4l2/v4l2-timeout.h"
#include <assert.h>

int main(void)
{
	assert(v4l2_capture_timeout(166666, 0, false) == 2000000);
	assert(v4l2_capture_timeout(166666, 1500000, false) == 500000);
	assert(v4l2_capture_timeout(166666, 1999999, false) == 166666);
	assert(v4l2_capture_timeout(166666, 2000000, false) == 166666);
	assert(v4l2_capture_timeout(166666, 100000, true) == 166666);
	assert(v4l2_capture_timeout(4000000, 0, false) == 4000000);
	return 0;
}
