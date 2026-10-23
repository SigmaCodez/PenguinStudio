// SPDX-License-Identifier: GPL-2.0-or-later
#include <obs-module.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>
#include <util/platform.h>
#include "segmenter.hpp"
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("penguin-camera", "en-US")

namespace {
using PenguinCamera::Segmenter;
struct Camera {
	obs_source_t *source;
	gs_effect_t *effect = nullptr;
	gs_texrender_t *capture = nullptr;
	gs_stagesurf_t *stage = nullptr;
	gs_texture_t *maskTexture = nullptr;
	std::atomic<int> shape{0}, background{0}, status{0};
	std::atomic<float> radius{0.25f}, similarity{0.15f};
	std::mutex mutex;
	std::condition_variable wake;
	std::thread worker;
	bool stopping = false, busy = false;
	uint64_t generation = 0, pendingGeneration = 0, outputGeneration = 0;
	std::vector<uint8_t> pending, output;
	bool newMask = false, maskReady = false;
	uint64_t textureGeneration = 0, lastCapture = 0;
	uint32_t lastWidth = 0, lastHeight = 0;
	std::string model;

	explicit Camera(obs_source_t *context) : source(context) {}
	void Run()
	{
		std::unique_ptr<Segmenter> segmenter;
		for (;;) {
			std::vector<uint8_t> frame;
			uint64_t version;
			{
				std::unique_lock lock(mutex);
				wake.wait(lock, [&] { return stopping || !pending.empty(); });
				if (stopping) {
					return;
				}
				frame.swap(pending);
				version = pendingGeneration;
			}
			try {
				if (!segmenter) {
					segmenter = std::make_unique<Segmenter>(model.c_str());
				}
				auto result = segmenter->Process(frame);
				std::lock_guard lock(mutex);
				if (generation == version && background == 1) {
					output = std::move(result);
					outputGeneration = version;
					newMask = true;
					status = 2;
				}
				busy = false;
			} catch (const std::exception &error) {
				blog(LOG_ERROR, "[Penguin Camera] Local background removal failed: %s", error.what());
				std::lock_guard lock(mutex);
				if (generation == version) {
					status = 3;
					++generation;
					newMask = false;
					output.clear();
				}
				busy = false;
			}
		}
	}
	~Camera()
	{
		{
			std::lock_guard lock(mutex);
			stopping = true;
		}
		wake.notify_one();
		if (worker.joinable()) {
			worker.join();
		}
		obs_enter_graphics();
		gs_texture_destroy(maskTexture);
		gs_stagesurface_destroy(stage);
		gs_texrender_destroy(capture);
		gs_effect_destroy(effect);
		obs_leave_graphics();
	}
};

const char *Name(void *)
{
	return obs_module_text("CameraEffects");
}
void Defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "radius", 25.0);
	obs_data_set_default_double(settings, "similarity", 15.0);
}
void Update(void *data, obs_data_t *settings)
{
	auto *camera = static_cast<Camera *>(data);
	camera->shape = std::clamp(int(obs_data_get_int(settings, "shape")), 0, 3);
	camera->radius = float(std::clamp(obs_data_get_double(settings, "radius"), 0.0, 100.0) / 100.0);
	camera->similarity = float(std::clamp(obs_data_get_double(settings, "similarity"), 0.0, 90.0) / 100.0);
	int background = std::clamp(int(obs_data_get_int(settings, "background")), 0, 3);
	if (background == 1 && std::strcmp(obs_data_get_string(settings, "automatic_backend"), "rvm") == 0) {
		background = 0;
	}
	std::lock_guard lock(camera->mutex);
	if (background != camera->background) {
		camera->background = background;
		++camera->generation;
		camera->output.clear();
		camera->newMask = false;
		camera->status = background == 1 ? 1 : 0;
	}
}
void Status(void *data, calldata_t *parameters)
{
	calldata_set_int(parameters, "status", static_cast<Camera *>(data)->status.load());
}
void *Create(obs_data_t *settings, obs_source_t *source)
{
	try {
		auto camera = std::make_unique<Camera>(source);
		char *path = obs_module_file("camera.effect");
		obs_enter_graphics();
		camera->effect = gs_effect_create_from_file(path, nullptr);
		camera->capture = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
		camera->stage = gs_stagesurface_create(Segmenter::Width, Segmenter::Height, GS_RGBA);
		obs_leave_graphics();
		bfree(path);
		if (!camera->effect || !camera->capture || !camera->stage) {
			return nullptr;
		}
		path = obs_module_file("selfie_segmentation.onnx");
		camera->model = path ? path : "";
		bfree(path);
		Update(camera.get(), settings);
		proc_handler_add(obs_source_get_proc_handler(source), "void camera_status(out int status)", Status,
				 camera.get());
		camera->worker = std::thread(&Camera::Run, camera.get());
		return camera.release();
	} catch (const std::exception &error) {
		blog(LOG_ERROR, "[Penguin Camera] Cannot create filter: %s", error.what());
		return nullptr;
	}
}
void Destroy(void *data)
{
	delete static_cast<Camera *>(data);
}

obs_properties_t *Properties(void *)
{
	auto *properties = obs_properties_create();
	auto *shape = obs_properties_add_list(properties, "shape", obs_module_text("Shape"), OBS_COMBO_TYPE_LIST,
					      OBS_COMBO_FORMAT_INT);
	const char *shapes[] = {"Rectangle", "Circle", "Oval", "Rounded"};
	for (int i = 0; i < 4; ++i) {
		obs_property_list_add_int(shape, obs_module_text(shapes[i]), i);
	}
	obs_properties_add_float_slider(properties, "radius", obs_module_text("Radius"), 0, 100, 1);
	auto *background = obs_properties_add_list(properties, "background", obs_module_text("Background"),
						   OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	const char *backgrounds[] = {"Off", "Automatic", "Green", "Blue"};
	for (int i = 0; i < 4; ++i) {
		obs_property_list_add_int(background, obs_module_text(backgrounds[i]), i);
	}
	obs_properties_add_float_slider(properties, "similarity", obs_module_text("Similarity"), 0, 90, 1);
	obs_properties_add_text(properties, "privacy", obs_module_text("Privacy"), OBS_TEXT_INFO);
	return properties;
}

// Capture only one small frame at a time. No inference or unbounded frame queue
// runs on OBS's graphics thread; skipped frames retain the latest finished mask.
void Capture(Camera *camera, obs_source_t *target, uint32_t width, uint32_t height)
{
	const auto now = os_gettime_ns();
	if (now - camera->lastCapture < 66666666 || camera->status == 3) {
		return;
	}
	uint64_t version;
	{
		std::lock_guard lock(camera->mutex);
		if (camera->busy) {
			return;
		}
		version = camera->generation;
	}
	gs_texrender_reset(camera->capture);
	if (!gs_texrender_begin(camera->capture, Segmenter::Width, Segmenter::Height)) {
		return;
	}
	vec4 clear{};
	gs_clear(GS_CLEAR_COLOR, &clear, 0, 0);
	gs_ortho(0, float(width), 0, float(height), -100, 100);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	const auto flags = obs_source_get_output_flags(target);
	if (target == obs_filter_get_parent(camera->source) && !(flags & (OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_ASYNC))) {
		obs_source_default_render(target);
	} else {
		obs_source_video_render(target);
	}
	gs_blend_state_pop();
	gs_texrender_end(camera->capture);
	gs_stage_texture(camera->stage, gs_texrender_get_texture(camera->capture));
	uint8_t *pixels;
	uint32_t stride;
	if (!gs_stagesurface_map(camera->stage, &pixels, &stride)) {
		return;
	}
	std::vector<uint8_t> frame(Segmenter::Width * Segmenter::Height * 4);
	for (uint32_t row = 0; row < Segmenter::Height; ++row) {
		memcpy(frame.data() + row * Segmenter::Width * 4, pixels + row * stride, Segmenter::Width * 4);
	}
	gs_stagesurface_unmap(camera->stage);
	{
		std::lock_guard lock(camera->mutex);
		if (version != camera->generation || camera->background != 1) {
			return;
		}
		camera->pending = std::move(frame);
		camera->pendingGeneration = version;
		camera->busy = true;
	}
	camera->lastCapture = now;
	camera->wake.notify_one();
}
void Render(void *data, gs_effect_t *)
{
	auto *camera = static_cast<Camera *>(data);
	auto *target = obs_filter_get_target(camera->source);
	const uint32_t width = target ? obs_source_get_base_width(target) : 0;
	const uint32_t height = target ? obs_source_get_base_height(target) : 0;
	if (!width || !height || (!camera->shape && !camera->background)) {
		obs_source_skip_video_filter(camera->source);
		return;
	}
	{
		std::lock_guard lock(camera->mutex);
		if (width != camera->lastWidth || height != camera->lastHeight) {
			++camera->generation;
			camera->newMask = false;
			if (camera->background == 1 && camera->status != 3) {
				camera->status = 1;
			}
			camera->lastWidth = width;
			camera->lastHeight = height;
		}
		if (camera->textureGeneration != camera->generation) {
			camera->maskReady = false;
		}
		if (camera->newMask && camera->outputGeneration == camera->generation) {
			const uint8_t *pixels = camera->output.data();
			if (!camera->maskTexture) {
				camera->maskTexture = gs_texture_create(Segmenter::Width, Segmenter::Height, GS_R8, 1,
									&pixels, GS_DYNAMIC);
			} else {
				gs_texture_set_image(camera->maskTexture, pixels, Segmenter::Width, false);
			}
			camera->maskReady = camera->maskTexture != nullptr;
			camera->textureGeneration = camera->generation;
			camera->newMask = false;
		}
	}
	if (camera->background == 1) {
		Capture(camera, target, width, height);
	}
	if (!obs_source_process_filter_begin(camera->source, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
		return;
	}
	const vec2 dimensions{float(width), float(height)};
	auto param = [&](const char *name) {
		return gs_effect_get_param_by_name(camera->effect, name);
	};
	gs_effect_set_vec2(param("dimensions"), &dimensions);
	gs_effect_set_int(param("shape"), camera->shape);
	gs_effect_set_int(param("background"), camera->background);
	gs_effect_set_float(param("radius"), camera->radius);
	gs_effect_set_float(param("similarity"), camera->similarity);
	gs_effect_set_bool(param("mask_ready"), camera->maskReady);
	gs_effect_set_texture(param("person_mask"), camera->maskTexture);
	obs_source_process_filter_end(camera->source, camera->effect, width, height);
}
} // namespace

bool obs_module_load(void)
{
	obs_source_info info{};
	info.id = "penguin_camera_filter";
	info.type = OBS_SOURCE_TYPE_FILTER;
	info.output_flags = OBS_SOURCE_VIDEO;
	info.get_name = Name;
	info.create = Create;
	info.destroy = Destroy;
	info.update = Update;
	info.get_defaults = Defaults;
	info.get_properties = Properties;
	info.video_render = Render;
	obs_register_source(&info);
	return true;
}
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Penguin Studio camera shapes and local portrait removal";
}
