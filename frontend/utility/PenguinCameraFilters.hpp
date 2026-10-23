// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <obs.hpp>
#include <algorithm>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace PenguinCamera {
// A pair must never be partially detached while the video thread renders it.
struct GraphicsGuard {
	GraphicsGuard() { obs_enter_graphics(); }
	~GraphicsGuard() { obs_leave_graphics(); }
	GraphicsGuard(const GraphicsGuard &) = delete;
	GraphicsGuard &operator=(const GraphicsGuard &) = delete;
};

inline constexpr const char *CameraFilterId = "penguin_camera_filter";

inline OBSSourceAutoRelease CameraFilter(obs_source_t *source)
{
	obs_source_t *result = nullptr;
	obs_source_enum_filters(
		source,
		[](obs_source_t *, obs_source_t *filter, void *data) {
			auto **found = static_cast<obs_source_t **>(data);
			if (!*found && std::strcmp(obs_source_get_id(filter), CameraFilterId) == 0) {
				*found = obs_source_get_ref(filter);
			}
		},
		&result);
	return OBSSourceAutoRelease(result);
}

// Only manage our companion filter; independently added filters belong to users.
inline OBSSourceAutoRelease CameraMatting(obs_source_t *source)
{
	obs_source_t *result = nullptr;
	obs_source_enum_filters(
		source,
		[](obs_source_t *, obs_source_t *filter, void *data) {
			if (std::strcmp(obs_source_get_id(filter), "background_removal") != 0) {
				return;
			}
			OBSDataAutoRelease settings = obs_source_get_settings(filter);
			auto **found = static_cast<obs_source_t **>(data);
			if (!*found && obs_data_get_bool(settings, "penguin_camera_managed")) {
				*found = obs_source_get_ref(filter);
			}
		},
		&result);
	return OBSSourceAutoRelease(result);
}

inline std::string SaveCamera(obs_source_t *source)
{
	OBSDataAutoRelease data = obs_data_create();
	obs_data_set_string(data, "source_uuid", obs_source_get_uuid(source));
	auto filter = CameraFilter(source);
	auto matting = CameraMatting(source);
	for (const auto &[key, current] : {std::pair{"filter", filter.Get()}, std::pair{"matting", matting.Get()}}) {
		if (current) {
			OBSDataAutoRelease saved = obs_save_source(current);
			obs_data_set_int(saved, "penguin_index", obs_source_filter_get_index(source, current));
			obs_data_set_obj(data, key, saved);
		}
	}
	return obs_data_get_json(data);
}

inline void RestoreCamera(const std::string &json)
{
	GraphicsGuard guard;
	OBSDataAutoRelease data = obs_data_create_from_json(json.c_str());
	OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(data, "source_uuid"));
	if (!source) {
		return;
	}
	auto filter = CameraFilter(source);
	auto matting = CameraMatting(source);
	for (const auto &[key, current] : {std::pair{"filter", filter.Get()}, std::pair{"matting", matting.Get()}}) {
		OBSDataAutoRelease saved = obs_data_get_obj(data, key);
		if (!saved) {
			if (current) {
				obs_source_filter_remove(source, current);
			}
		} else if (current) {
			OBSDataAutoRelease settings = obs_data_get_obj(saved, "settings");
			obs_source_reset_settings(current, settings);
			obs_source_set_enabled(current, obs_data_get_bool(saved, "enabled"));
		} else {
			OBSSourceAutoRelease restored = obs_load_source(saved);
			if (restored) {
				obs_source_filter_add(source, restored);
			}
		}
	}
	// Restore positions after both filters exist, in ascending index order.
	filter = CameraFilter(source);
	matting = CameraMatting(source);
	std::vector<std::pair<int, obs_source_t *>> positions;
	for (const auto &[key, current] : {std::pair{"filter", filter.Get()}, std::pair{"matting", matting.Get()}}) {
		OBSDataAutoRelease saved = obs_data_get_obj(data, key);
		if (current && saved) {
			positions.emplace_back(int(obs_data_get_int(saved, "penguin_index")), current);
		}
	}
	std::sort(positions.begin(), positions.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
	for (const auto &[index, current] : positions) {
		obs_source_filter_set_index(source, current, index);
	}
}

inline bool Apply(obs_source_t *source, const char *name, int shape, int background, double radius, double similarity)
{
	GraphicsGuard guard;
	const auto undo = SaveCamera(source);
	auto filter = CameraFilter(source);
	auto matting = CameraMatting(source);
	const bool automatic = background == 1;
	if (automatic && !matting) {
		OBSDataAutoRelease mattingSettings = obs_data_create();
		obs_data_set_bool(mattingSettings, "penguin_camera_managed", true);
		obs_data_set_string(mattingSettings, "model_select",
				    "models/rvm_mobilenetv3_fp32.with_runtime_opt.ort");
		obs_data_set_string(mattingSettings, "useGPU", "cpu");
		obs_data_set_int(mattingSettings, "numThreads", 2);
		obs_data_set_bool(mattingSettings, "enable_threshold", false);
		obs_data_set_bool(mattingSettings, "enable_image_similarity", false);
		obs_data_set_double(mattingSettings, "temporal_smooth_factor", 0.0);
		matting = obs_source_create("background_removal", "Penguin Camera Matting", mattingSettings, nullptr);
		if (!matting) {
			return false;
		}
		obs_source_filter_add(source, matting);
	}
	OBSDataAutoRelease settings = filter ? obs_source_get_settings(filter) : obs_data_create();
	obs_data_set_int(settings, "shape", shape);
	obs_data_set_int(settings, "background", background);
	obs_data_set_string(settings, "automatic_backend", automatic ? "rvm" : "");
	obs_data_set_double(settings, "radius", radius);
	obs_data_set_double(settings, "similarity", similarity);
	if (filter) {
		obs_source_update(filter, settings);
		obs_source_set_enabled(filter, true);
	} else {
		filter = obs_source_create(CameraFilterId, name, settings, nullptr);
		if (!filter) {
			RestoreCamera(undo);
			return false;
		}
		obs_source_filter_add(source, filter);
	}
	if (matting) {
		if (automatic) {
			obs_source_set_enabled(matting, true);
			const int cameraIndex = obs_source_filter_get_index(source, filter);
			const int mattingIndex = obs_source_filter_get_index(source, matting);
			// Matting must see the full frame; apply the shape to its output.
			obs_source_filter_set_index(source, matting,
						    cameraIndex + 1 - (mattingIndex < cameraIndex ? 1 : 0));
		} else {
			obs_source_filter_remove(source, matting);
		}
	}
	return true;
}
} // namespace PenguinCamera
