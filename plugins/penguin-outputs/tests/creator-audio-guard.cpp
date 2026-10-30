// SPDX-License-Identifier: GPL-2.0-or-later
// No devices, recordings, accounts or graphics context are used by this check.
#include "../creator-tools.cpp"
#include <array>
#include <cassert>
int main()
{
	std::array<std::array<float, 17>, MAX_AV_PLANES> planes{};
	obs_audio_data audio{};
	audio.frames = 16;
	audio.timestamp = 123456;
	for (size_t channel = 0; channel < planes.size(); ++channel) {
		planes[channel].fill(0.75f);
		audio.data[channel] = reinterpret_cast<uint8_t *>(planes[channel].data());
	}
	privacy.store(false);
	assert(SilenceGuard(nullptr, &audio) == &audio);
	for (const auto &plane : planes)
		for (float sample : plane)
			assert(sample == 0.75f);
	privacy.store(true);
	assert(SilenceGuard(nullptr, &audio) == &audio);
	assert(audio.timestamp == 123456 && audio.frames == 16);
	for (const auto &plane : planes) {
		for (size_t sample = 0; sample < 16; ++sample)
			assert(plane[sample] == 0.0f);
		assert(plane[16] == 0.75f); // Guard against overwriting the next audio block.
	}
	audio.data[1] = nullptr;
	audio.frames = 0;
	assert(SilenceGuard(nullptr, &audio) == &audio);
	privacy.store(false);
	const auto help = ExplainOutputFailure("403 unauthorized rtmps://private.example/live/SECRET-TEST-KEY", false);
	assert(help.contains("authorization"));
	assert(!help.contains("SECRET-TEST-KEY") && !help.contains("private.example"));
	assert(ExplainOutputFailure("cannot write /private/example: no space", true).contains("storage"));
	assert(ExplainOutputFailure("nvenc initialization failed", false).contains("encoder"));
	assert(ExplainOutputFailure(nullptr, false).isEmpty());
	assert(RecordingDuration(900, 30, 1) == "00:00:30");
	assert(RecordingDuration(108000, 30, 1) == "01:00:00");
	assert(RecordingDuration(30000, 30000, 1001) == "00:16:41");
	assert(RecordingDuration(1, 0, 1) == "Unavailable");
	assert(RecordingDuration(-1, 30, 1) == "Unavailable");
	assert(RecordingFormatAdvice("mp4", false).contains("unusable"));
	assert(RecordingFormatAdvice("hybrid_mp4", false).contains("supports recovery"));
	assert(RecordingFormatAdvice("mkv", true).contains("Custom recording"));
	assert(!RecordingFormatAdvice("PRIVATE-PATH", false).contains("PRIVATE-PATH"));
	config_t *config = nullptr;
	assert(config_open_string(&config, "") == CONFIG_SUCCESS);
	assert(RecordingDirectory(nullptr).isEmpty());
	config_set_string(config, "SimpleOutput", "FilePath", "/simple");
	assert(RecordingDirectory(config) == "/simple");
	config_set_string(config, "Output", "Mode", "Advanced");
	config_set_string(config, "AdvOut", "RecFilePath", "/standard");
	assert(RecordingDirectory(config) == "/standard");
	config_set_string(config, "AdvOut", "RecType", "FFmpeg");
	config_set_string(config, "AdvOut", "FFFilePath", "/custom");
	assert(RecordingDirectory(config).isEmpty());
	config_set_bool(config, "AdvOut", "FFOutputToFile", true);
	assert(RecordingDirectory(config) == "/custom");
	config_close(config);
	return 0;
}
