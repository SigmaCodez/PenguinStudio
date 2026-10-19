// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <string>
#include <string_view>

namespace PenguinStudio {

// Keep OBS's internal interfaces compatible while isolating Penguin Studio's files.
inline std::string ConfigName(const char *name)
{
	std::string_view path = name ? name : "";
	constexpr std::string_view upstreamDirectory = "obs-studio";
	if (path == upstreamDirectory || path.substr(0, upstreamDirectory.size() + 1) == "obs-studio/") {
		return "PenguinStudio" + std::string(path.substr(upstreamDirectory.size()));
	}
	return std::string(path);
}

} // namespace PenguinStudio
