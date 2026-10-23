// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/penguin-backgroundremoval/managed-threads.hpp"
#include <cassert>

int main()
{
	for (int cores : {0, 1, 2, 4, 8, 64}) {
		obs_data_t *s = obs_data_create();
		obs_data_set_default_int(s, "numThreads", 1);
		penguin_configure_managed_threads(s, cores);
		assert(!obs_data_has_user_value(s, "numThreads"));
		obs_data_set_bool(s, "penguin_camera_managed", true);
		penguin_configure_managed_threads(s, cores);
		assert(obs_data_get_int(s, "numThreads") == std::clamp(cores, 1, 4));
		obs_data_set_int(s, "numThreads", 2);
		penguin_configure_managed_threads(s, 8);
		assert(obs_data_get_int(s, "numThreads") == 2); // Subsequent manual choice.
		obs_data_release(s);
	}
	for (int threads : {1, 2, 3, 4, 8}) {
		obs_data_t *s = obs_data_create();
		obs_data_set_bool(s, "penguin_camera_managed", true);
		obs_data_set_int(s, "numThreads", threads);
		penguin_configure_managed_threads(s, 8);
		assert(obs_data_get_int(s, "numThreads") == (threads == 2 ? 4 : threads));
		assert(obs_data_get_int(s, "penguin_thread_policy") == 1);
		obs_data_release(s);
	}
}
