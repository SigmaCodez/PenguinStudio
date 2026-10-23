// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <obs-data.h>
#include <algorithm>

inline void penguin_configure_managed_threads(obs_data_t *settings, int physicalCores)
{
	if (!obs_data_get_bool(settings, "penguin_camera_managed") ||
	    obs_data_get_int(settings, "penguin_thread_policy") >= 1)
		return;

	// Upgrade the original managed default once. Leave explicit non-default
	// choices and all independently added filters under the user's control.
	if (!obs_data_has_user_value(settings, "numThreads") || obs_data_get_int(settings, "numThreads") == 2)
		obs_data_set_int(settings, "numThreads", std::clamp(physicalCores, 1, 4));
	obs_data_set_int(settings, "penguin_thread_policy", 1);
}
