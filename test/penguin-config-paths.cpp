// SPDX-License-Identifier: GPL-2.0-or-later
#include "frontend/utility/PenguinBranding.hpp"

#include <cassert>

int main()
{
	using PenguinStudio::ConfigName;
	assert(ConfigName(nullptr).empty());
	assert(ConfigName("").empty());
	assert(ConfigName("obs-studio") == "PenguinStudio");
	assert(ConfigName("obs-studio/global.ini") == "PenguinStudio/global.ini");
	assert(ConfigName("obs-studio/basic/profiles/Stream/basic.ini") ==
	       "PenguinStudio/basic/profiles/Stream/basic.ini");
	assert(ConfigName("obs-studio/plugin_config/penguin-outputs.json") ==
	       "PenguinStudio/plugin_config/penguin-outputs.json");
	assert(ConfigName("PenguinStudio/user.ini") == "PenguinStudio/user.ini");
	assert(ConfigName("obs-studio-other/settings") == "obs-studio-other/settings");
	assert(ConfigName("custom/obs-studio") == "custom/obs-studio");
}
