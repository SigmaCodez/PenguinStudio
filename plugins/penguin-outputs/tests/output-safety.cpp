// SPDX-License-Identifier: GPL-2.0-or-later
// Compile this translation unit with function/data sections and --gc-sections.
// These doubles exercise validation, persistence and destruction order without a network or OBS session.
#include "../penguin-outputs.cpp"

#include <cstdlib>
#include <iostream>

namespace {
bool encoderAvailable = true;
std::vector<std::string> lifecycle;

void Require(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		std::exit(EXIT_FAILURE);
	}
}

void Populate(RunningOutput &running)
{
	running.output = reinterpret_cast<obs_output_t *>(1);
	running.video = reinterpret_cast<obs_encoder_t *>(2);
	running.audio = reinterpret_cast<obs_encoder_t *>(3);
	running.service = reinterpret_cast<obs_service_t *>(4);
	running.canvas = reinterpret_cast<obs_canvas_t *>(5);
	running.scene = reinterpret_cast<obs_scene_t *>(6);
}
} // namespace

extern "C" {
bool text_lookup_getstr(lookup_t *, const char *key, const char **out)
{
	*out = key;
	return true;
}
bool calldata_get_data(const calldata_t *, const char *, void *out, size_t size)
{
	std::memset(out, 0, size);
	return true;
}
bool obs_enum_encoder_types(size_t index, const char **id)
{
	if (!encoderAvailable || index != 0)
		return false;
	*id = "obs_x264";
	return true;
}
const char *obs_get_encoder_codec(const char *)
{
	return "h264";
}
signal_handler_t *obs_output_get_signal_handler(const obs_output_t *)
{
	return nullptr;
}
void signal_handler_disconnect(signal_handler_t *, const char *, signal_callback_t, void *) {}
void obs_output_force_stop(obs_output_t *) { lifecycle.emplace_back("force-stop"); }
void obs_output_release(obs_output_t *) { lifecycle.emplace_back("release-output"); }
void obs_encoder_release(obs_encoder_t *) { lifecycle.emplace_back("release-encoder"); }
void obs_service_release(obs_service_t *) { lifecycle.emplace_back("release-service"); }
void obs_canvas_set_channel(obs_canvas_t *, uint32_t, obs_source_t *) { lifecycle.emplace_back("clear-channel"); }
void obs_scene_release(obs_scene_t *) { lifecycle.emplace_back("release-scene"); }
void obs_canvas_release(obs_canvas_t *) { lifecycle.emplace_back("release-canvas"); }
}

int main()
{
	Destination destination;
	destination.name = QStringLiteral("Offline test");
	destination.server = QStringLiteral("rtmps://example.invalid/live");
	destination.key = QStringLiteral("DO-NOT-SAVE-this-stream-key");
	Require(Validate(destination, true).isEmpty(), "valid destination must be accepted");
	for (const char *server : {"https://example.invalid/live", "rtmp:///missing-host", "rtmps://user:secret@example.invalid/live",
				   "rtmps://example.invalid/live?token=secret", "rtmps://example.invalid/live#secret",
				   "rtmp://example.invalid/live path"}) {
		destination.server = QString::fromUtf8(server);
		Require(!Validate(destination, true).isEmpty(), "invalid or credential-bearing server must be rejected");
	}
	destination.server = QStringLiteral("rtmp://127.0.0.1:1935/live");
	Require(Validate(destination, true).isEmpty(), "local RTMP sink must be accepted");
	const QJsonObject saved = SavedDestination(destination);
	Require(!saved.contains(QStringLiteral("key")), "stream key field must not be serialized");
	Require(!QJsonDocument(saved).toJson().contains(destination.key.toUtf8()), "stream key content must not be serialized");
	destination.key.clear();
	Require(!Validate(destination, true).isEmpty(), "starting without a key must fail");
	Require(Validate(destination, false).isEmpty(), "saving a destination before supplying a key must work");
	encoderAvailable = false;
	Require(!Validate(destination, false).isEmpty(), "missing encoder must fail preflight");

	{
		RunningOutput failed;
		Populate(failed);
	}
	Require(lifecycle.front() == "release-output", "a never-started output must not be force-stopped");
	Require(lifecycle.back() == "release-canvas", "canvas must outlive output and encoders");
	Require(lifecycle[1] == "release-encoder" && lifecycle[2] == "release-encoder",
		"encoder references must be released after output destruction");
	lifecycle.clear();
	{
		RunningOutput live;
		Populate(live);
		live.startRequested = true;
	}
	Require(lifecycle[0] == "force-stop" && lifecycle[1] == "release-output",
		"active output must be stopped before releasing its resources");
	lifecycle.clear();
	{
		RunningOutput ended;
		Populate(ended);
		ended.startRequested = true;
		ended.stopped.store(true);
	}
	Require(lifecycle.front() == "release-output", "already-stopped output must not be stopped again");
	std::cout << "PASS: destination validation, key omission and failed/live/stopped output destruction\n";
}
