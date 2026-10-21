// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <obs.hpp>
#include <cstring>

// Query the driver's capabilities for the encoder's default device and profile.
// Simple output needs CBR for streaming and CQP for quality-based recording.
inline bool PenguinVAAPISupportsSimpleOutput(obs_properties_t *properties, obs_data_t *settings)
{
	if (!settings || !properties) {
		return false;
	}
	obs_properties_apply_settings(properties, settings);
	obs_property_t *rates = obs_properties_get(properties, "rate_control");
	if (!rates) {
		return false;
	}
	bool cbr = false;
	bool cqp = false;
	for (size_t i = 0; i < obs_property_list_item_count(rates); ++i) {
		if (obs_property_list_item_disabled(rates, i)) {
			continue;
		}
		const char *value = obs_property_list_item_string(rates, i);
		if (!value) {
			continue;
		}
		cbr |= std::strcmp(value, "CBR") == 0;
		cqp |= std::strcmp(value, "CQP") == 0;
	}
	return cbr && cqp;
}

inline bool PenguinVAAPIAvailable()
{
	OBSDataAutoRelease settings = obs_encoder_defaults("ffmpeg_vaapi_tex");
	OBSProperties properties = obs_get_encoder_properties("ffmpeg_vaapi_tex");
	return PenguinVAAPISupportsSimpleOutput(properties, settings);
}
