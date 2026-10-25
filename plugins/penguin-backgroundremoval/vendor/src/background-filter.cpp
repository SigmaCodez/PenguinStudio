// SPDX-FileCopyrightText: 2021-2026 Roy Shilkrot <roy.shil@gmail.com>
// SPDX-FileCopyrightText: 2023-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "background-filter.h"

#include <onnxruntime_cxx_api.h>

#ifdef _WIN32
#include <wchar.h>
#endif // _WIN32

#include <opencv2/core/version.hpp>

#if CV_VERSION_MAJOR >= 5
#include <opencv2/geometry.hpp>
#endif

#include <opencv2/imgproc.hpp>

#include <numeric>
#include <memory>
#include <exception>
#include <fstream>
#include <new>
#include <mutex>
#include <regex>
#include <thread>

#include "plugin-support.h"
#include "models/ModelSINET.hpp"
#include "models/ModelMediapipe.hpp"
#include "models/ModelSelfie.hpp"
#include "models/ModelSelfieMulticlass.hpp"
#include "models/ModelRVM.hpp"
#include "models/ModelPPHumanSeg.hpp"
#include "models/ModelTCMonoDepth.hpp"
#include "FilterData.hpp"
#include "ort-utils/ort-session-utils.hpp"
#include "obs-utils/obs-utils.hpp"
#include "consts.h"
#include "../../managed-threads.hpp"
#include "../../gpu-worker-client.hpp"
#include <filesystem>
#include <util/platform.h>
// Penguin Studio manages component updates.

struct background_removal_filter : public filter_data, public std::enable_shared_from_this<background_removal_filter> {
	std::unique_ptr<PenguinGPUWorker> gpuWorker;
	std::atomic<bool> gpuPreparing{false};
	bool enableThreshold = true;
	bool stopWhenSourceIsInactive = true;
	float threshold = 0.5f;
	cv::Scalar backgroundColor{0, 0, 0, 0};
	float contourFilter = 0.05f;
	float smoothContour = 0.5f;
	float feather = 0.0f;
	int maskExpansion = 0;

	cv::Mat backgroundMask;
	cv::Mat lastBackgroundMask;
	cv::Mat lastImageBGRA;
	float temporalSmoothFactor = 0.0f;
	float imageSimilarityThreshold = 35.0f;
	bool enableImageSimilarity = true;
	int maskEveryXFrames = 1;
	int maskEveryXFramesCount = 0;
	int64_t blurBackground = 0;
	bool enableFocalBlur = false;
	float blurFocusPoint = 0.1f;
	float blurFocusDepth = 0.1f;

	gs_effect_t *effect;
	gs_effect_t *kawaseBlurEffect;

	std::mutex modelMutex;
	std::atomic<int> penguinStatus{1};
	bool maskPending = true;

	~background_removal_filter() { obs_log(LOG_INFO, "Background removal filter destructor called"); }
};

void background_removal_thread(void *data); // Forward declaration

const char *background_filter_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("BackgroundRemoval");
}

/**                   PROPERTIES                     */

static bool visible_on_bool(obs_properties_t *ppts, obs_data_t *settings, const char *bool_prop, const char *prop_name)
{
	const bool enabled = obs_data_get_bool(settings, bool_prop);
	obs_property_t *p = obs_properties_get(ppts, prop_name);
	obs_property_set_visible(p, enabled);
	return true;
}

static bool enable_threshold_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	return visible_on_bool(ppts, settings, "enable_threshold", "threshold_group");
}

static bool enable_focal_blur(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	return visible_on_bool(ppts, settings, "enable_focal_blur", "focal_blur_group");
}

static bool enable_image_similarity(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	return visible_on_bool(ppts, settings, "enable_image_similarity", "image_similarity_threshold");
}

static bool enable_advanced_settings(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	const bool enabled = obs_data_get_bool(settings, "advanced");
	p = obs_properties_get(ppts, "blur_background");
	obs_property_set_visible(p, true);

	for (const char *prop_name :
	     {"model_select", "useGPU", "mask_every_x_frames", "numThreads", "enable_focal_blur", "enable_threshold",
	      "threshold_group", "focal_blur_group", "temporal_smooth_factor", "image_similarity_threshold",
	      "enable_image_similarity", "mask_expansion"}) {
		p = obs_properties_get(ppts, prop_name);
		obs_property_set_visible(p, enabled);
	}

	if (enabled) {
		enable_threshold_modified(ppts, p, settings);
		enable_focal_blur(ppts, p, settings);
		enable_image_similarity(ppts, p, settings);
	}

	return true;
}

obs_properties_t *background_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *advanced = obs_properties_add_bool(props, "advanced", obs_module_text("Advanced"));

	obs_properties_add_bool(props, "stop_when_source_is_inactive",
				obs_module_text("Stop filter when source is inactive"));

	// If advanced is selected show the advanced settings, otherwise hide them
	obs_property_set_modified_callback(advanced, enable_advanced_settings);

	/* Threshold props */
	obs_property_t *p_enable_threshold =
		obs_properties_add_bool(props, "enable_threshold", obs_module_text("EnableThreshold"));
	obs_property_set_modified_callback(p_enable_threshold, enable_threshold_modified);

	// Threshold props group
	obs_properties_t *threshold_props = obs_properties_create();

	obs_properties_add_float_slider(threshold_props, "threshold", obs_module_text("Threshold"), 0.0, 1.0, 0.025);

	obs_properties_add_float_slider(threshold_props, "contour_filter",
					obs_module_text("ContourFilterPercentOfImage"), 0.0, 1.0, 0.025);

	obs_properties_add_float_slider(threshold_props, "smooth_contour", obs_module_text("SmoothSilhouette"), 0.0,
					1.0, 0.05);

	obs_properties_add_float_slider(threshold_props, "feather", obs_module_text("FeatherBlendSilhouette"), 0.0, 1.0,
					0.05);

	obs_properties_add_group(props, "threshold_group", obs_module_text("ThresholdGroup"), OBS_GROUP_NORMAL,
				 threshold_props);

	/* Mask expansion slider - in advanced settings */
	obs_properties_add_int_slider(props, "mask_expansion", obs_module_text("MaskExpansion"), -30, 30, 1);

	/* GPU, CPU and performance Props */
	obs_property_t *p_use_gpu = obs_properties_add_list(props, "useGPU", obs_module_text("InferenceDevice"),
							    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p_use_gpu, obs_module_text("AutomaticDevice"), "auto");
	obs_property_list_add_string(p_use_gpu, "GPU", "gpu");
	obs_property_list_add_string(p_use_gpu, "CPU", "cpu");

	obs_properties_add_int(props, "mask_every_x_frames", obs_module_text("CalculateMaskEveryXFrame"), 1, 300, 1);
	obs_properties_add_int_slider(props, "numThreads", obs_module_text("NumThreads"), 0, 8, 1);

	/* Model selection Props */
	obs_property_t *p_model_select = obs_properties_add_list(props, "model_select",
								 obs_module_text("SegmentationModel"),
								 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p_model_select, obs_module_text("Robust Video Matting"), MODEL_RVM);

	obs_properties_add_float_slider(props, "temporal_smooth_factor", obs_module_text("TemporalSmoothFactor"), 0.0,
					1.0, 0.01);

	obs_property_t *p_enable_image_similarity =
		obs_properties_add_bool(props, "enable_image_similarity", obs_module_text("EnableImageSimilarity"));
	obs_property_set_modified_callback(p_enable_image_similarity, enable_image_similarity);

	obs_properties_add_float_slider(props, "image_similarity_threshold",
					obs_module_text("ImageSimilarityThreshold"), 0.0, 100.0, 1.0);

	/* Background Blur Props */
	obs_properties_add_int_slider(props, "blur_background", obs_module_text("BlurBackgroundFactor0NoBlurUseColor"),
				      0, 20, 1);

	obs_property_t *p_enable_focal_blur =
		obs_properties_add_bool(props, "enable_focal_blur", obs_module_text("EnableFocalBlur"));
	obs_property_set_modified_callback(p_enable_focal_blur, enable_focal_blur);

	obs_properties_t *focal_blur_props = obs_properties_create();

	obs_properties_add_float_slider(focal_blur_props, "blur_focus_point", obs_module_text("BlurFocusPoint"), 0.0,
					1.0, 0.05);
	obs_properties_add_float_slider(focal_blur_props, "blur_focus_depth", obs_module_text("BlurFocusDepth"), 0.0,
					0.3, 0.02);

	obs_properties_add_group(props, "focal_blur_group", obs_module_text("FocalBlurGroup"), OBS_GROUP_NORMAL,
				 focal_blur_props);

	// Add a informative text about the plugin
	// replace the placeholder with the current version
	// use std::regex_replace instead of QString::arg because the latter doesn't work on Linux
	std::string basic_info = std::regex_replace(PLUGIN_INFO_TEMPLATE, std::regex("%1"), PLUGIN_VERSION);
	obs_properties_add_text(props, "info", basic_info.c_str(), OBS_TEXT_INFO);

	UNUSED_PARAMETER(data);
	return props;
}

void background_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "advanced", false);
	obs_data_set_default_bool(settings, "stop_when_source_is_inactive", true);
	obs_data_set_default_bool(settings, "enable_threshold", true);
	obs_data_set_default_double(settings, "threshold", 0.5);
	obs_data_set_default_double(settings, "contour_filter", 0.05);
	obs_data_set_default_double(settings, "smooth_contour", 0.5);
	obs_data_set_default_double(settings, "mask_expansion", 0);
	obs_data_set_default_double(settings, "feather", 0.0);
#if defined(__APPLE__)
	obs_data_set_default_string(settings, "useGPU", "auto");
#else
	// Linux
	obs_data_set_default_string(settings, "useGPU", "auto");
#endif
	obs_data_set_default_string(settings, "model_select", MODEL_RVM);
	obs_data_set_default_int(settings, "mask_every_x_frames", 1);
	obs_data_set_default_int(settings, "blur_background", 0);
	obs_data_set_default_int(settings, "numThreads", 1);
	obs_data_set_default_bool(settings, "enable_focal_blur", false);
	obs_data_set_default_double(settings, "temporal_smooth_factor", 0.85);
	obs_data_set_default_double(settings, "image_similarity_threshold", 35.0);
	obs_data_set_default_bool(settings, "enable_image_similarity", true);
	obs_data_set_default_double(settings, "blur_focus_point", 0.1);
	obs_data_set_default_double(settings, "blur_focus_depth", 0.0);
}

void background_filter_update(void *data, obs_data_t *settings)
{
	obs_log(LOG_INFO, "Background filter updated");

	// Cast to shared_ptr pointer and create a local shared_ptr
	auto *ptr = static_cast<std::shared_ptr<background_removal_filter> *>(data);
	if (!ptr) {
		return;
	}

	std::shared_ptr<background_removal_filter> tf = *ptr;
	if (!tf) {
		return;
	}

	tf->isDisabled = true;

	tf->stopWhenSourceIsInactive = obs_data_get_bool(settings, "stop_when_source_is_inactive");
	tf->enableThreshold = (float)obs_data_get_bool(settings, "enable_threshold");
	tf->threshold = (float)obs_data_get_double(settings, "threshold");

	tf->contourFilter = (float)obs_data_get_double(settings, "contour_filter");
	tf->smoothContour = (float)obs_data_get_double(settings, "smooth_contour");
	tf->maskExpansion = (int)obs_data_get_double(settings, "mask_expansion");
	tf->feather = (float)obs_data_get_double(settings, "feather");
	tf->maskEveryXFrames = (int)obs_data_get_int(settings, "mask_every_x_frames");
	tf->maskEveryXFramesCount = (int)(0);
	tf->blurBackground = obs_data_get_int(settings, "blur_background");
	tf->enableFocalBlur = (float)obs_data_get_bool(settings, "enable_focal_blur");
	tf->blurFocusPoint = (float)obs_data_get_double(settings, "blur_focus_point");
	tf->blurFocusDepth = (float)obs_data_get_double(settings, "blur_focus_depth");
	tf->temporalSmoothFactor = (float)obs_data_get_double(settings, "temporal_smooth_factor");
	tf->imageSimilarityThreshold = (float)obs_data_get_double(settings, "image_similarity_threshold");
	tf->enableImageSimilarity = (float)obs_data_get_bool(settings, "enable_image_similarity");

	const std::string newUseGpu = obs_data_get_string(settings, "useGPU");
	const std::string newModel = obs_data_get_string(settings, "model_select");
	penguin_configure_managed_threads(settings, os_get_physical_cores());
	const bool preserveHands = obs_data_get_bool(settings, "penguin_preserve_hands") && newModel == MODEL_RVM;
	const uint32_t configuredThreads = (uint32_t)obs_data_get_int(settings, "numThreads");
	const uint32_t newNumThreads = preserveHands ? std::clamp(configuredThreads, 1u, 3u) : configuredThreads;

	if (!tf->model || tf->modelSelection.empty() || tf->modelSelection != newModel || tf->useGPU != newUseGpu ||
	    tf->numThreads != newNumThreads || tf->preserveHands != preserveHands) {
		// lock modelMutex
		std::unique_lock<std::mutex> lock(tf->modelMutex);

		tf->penguinStatus = 1;
		// Re-initialize model if it's not already the selected one or switching inference device
		tf->modelSelection = newModel;
		tf->useGPU = newUseGpu;
		tf->numThreads = newNumThreads;
		tf->preserveHands = preserveHands;

		if (tf->modelSelection == MODEL_SINET) {
			tf->model.reset(new ModelSINET);
		}
		if (tf->modelSelection == MODEL_SELFIE) {
			tf->model.reset(new ModelSelfie);
		}
		if (tf->modelSelection == MODEL_SELFIE_MULTICLASS) {
			tf->model.reset(new ModelSelfieMulticlass);
		}
		if (tf->modelSelection == MODEL_MEDIAPIPE) {
			tf->model.reset(new ModelMediaPipe);
		}
		if (tf->modelSelection == MODEL_RVM) {
			tf->model.reset(new ModelRVM(tf->preserveHands, int(tf->numThreads), "cpu"));
		}
		if (tf->modelSelection == MODEL_PPHUMANSEG) {
			tf->model.reset(new ModelPPHumanSeg);
		}
		if (tf->modelSelection == MODEL_DEPTH_TCMONODEPTH) {
			tf->model.reset(new ModelTCMonoDepth);
		}

		int ortSessionResult = createOrtSession(tf.get());
		if (ortSessionResult != OBS_BGREMOVAL_ORT_SESSION_SUCCESS) {
			obs_log(LOG_ERROR, "Failed to create ONNXRuntime session. Error code: %d", ortSessionResult);
			// disable filter
			tf->penguinStatus = 3;
			tf->isDisabled = true;
			tf->model.reset();
			return;
		}
		tf->gpuWorker.reset();
		tf->activeGPU = false;
		tf->cpuFallback = tf->useGPU != "cpu";
		tf->gpuPreparing = false;
		if (tf->useGPU != "cpu" && tf->modelSelection == MODEL_RVM) {
			const auto module =
				std::filesystem::path(obs_get_module_binary_path(obs_current_module())).parent_path();
			const auto runtime = module.parent_path() / "penguin-camera-gpu/libonnxruntime.so.1";
			const auto executable = module.parent_path() / "penguin-camera/penguin-gpu-worker";
			if (std::filesystem::is_regular_file(runtime) && std::filesystem::is_regular_file(executable)) {
				char *models = obs_module_file("models");
				char *cache = obs_module_config_path("gpu-cache/onnxruntime-1.23.2");
				if (models && cache) {
					tf->gpuWorker = std::make_unique<PenguinGPUWorker>(executable.string(),
											   runtime.string(), models,
											   cache, int(tf->numThreads),
											   false);
					tf->gpuPreparing = tf->gpuWorker->Preparing();
					tf->cpuFallback = tf->gpuWorker->Failed();
				}
				bfree(models);
				bfree(cache);
			}
		}
	}

	obs_enter_graphics();

	char *effect_path = obs_module_file(EFFECT_PATH);
	gs_effect_destroy(tf->effect);
	tf->effect = gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);

	char *kawaseBlurEffectPath = obs_module_file(KAWASE_BLUR_EFFECT_PATH);
	gs_effect_destroy(tf->kawaseBlurEffect);
	tf->kawaseBlurEffect = gs_effect_create_from_file(kawaseBlurEffectPath, NULL);
	bfree(kawaseBlurEffectPath);

	obs_leave_graphics();

	// Log the currently selected options
	obs_log(LOG_INFO, "Background Removal Filter Options:");
	// name of the source that the filter is attached to
	obs_log(LOG_INFO, "  Source: %s", obs_source_get_name(tf->source));
	obs_log(LOG_INFO, "  Model: %s", tf->modelSelection.c_str());
	obs_log(LOG_INFO, "  Inference Device: %s", tf->useGPU.c_str());
	obs_log(LOG_INFO, "  Num Threads: %d", tf->numThreads);
	obs_log(LOG_INFO, "  Enable Threshold: %s", tf->enableThreshold ? "true" : "false");
	obs_log(LOG_INFO, "  Threshold: %f", tf->threshold);
	obs_log(LOG_INFO, "  Contour Filter: %f", tf->contourFilter);
	obs_log(LOG_INFO, "  Smooth Contour: %f", tf->smoothContour);
	obs_log(LOG_INFO, "  Mask Expansion: %f", tf->maskExpansion);
	obs_log(LOG_INFO, "  Feather: %f", tf->feather);
	obs_log(LOG_INFO, "  Mask Every X Frames: %d", tf->maskEveryXFrames);
	obs_log(LOG_INFO, "  Enable Image Similarity: %s", tf->enableImageSimilarity ? "true" : "false");
	obs_log(LOG_INFO, "  Image Similarity Threshold: %f", tf->imageSimilarityThreshold);
	obs_log(LOG_INFO, "  Blur Background: %d", tf->blurBackground);
	obs_log(LOG_INFO, "  Enable Focal Blur: %s", tf->enableFocalBlur ? "true" : "false");
	obs_log(LOG_INFO, "  Blur Focus Point: %f", tf->blurFocusPoint);
	obs_log(LOG_INFO, "  Blur Focus Depth: %f", tf->blurFocusDepth);
	obs_log(LOG_INFO, "  Disabled: %s", tf->isDisabled ? "true" : "false");
#ifdef _WIN32
	obs_log(LOG_INFO, "  Model file path: %S", tf->modelFilepath.c_str());
#else
	obs_log(LOG_INFO, "  Model file path: %s", tf->modelFilepath.c_str());
#endif

	// enable
	tf->isDisabled = false;
}

void background_filter_activate(void *data)
{
	auto *ptr = static_cast<std::shared_ptr<background_removal_filter> *>(data);
	if (!ptr) {
		return;
	}

	std::shared_ptr<background_removal_filter> tf = *ptr;
	if (tf && tf->stopWhenSourceIsInactive && tf->penguinStatus != 3) {
		obs_log(LOG_INFO, "Background filter activated");
		tf->isDisabled = false;
	}
}

void background_filter_deactivate(void *data)
{
	auto *ptr = static_cast<std::shared_ptr<background_removal_filter> *>(data);
	if (!ptr) {
		return;
	}

	std::shared_ptr<background_removal_filter> tf = *ptr;
	if (tf && tf->stopWhenSourceIsInactive) {
		obs_log(LOG_INFO, "Background filter deactivated");
		tf->isDisabled = true;
	}
}

/**                   FILTER CORE                     */

void *background_filter_create(obs_data_t *settings, obs_source_t *source)
{
	obs_log(LOG_INFO, "Background filter created");
	try {
		// Create the instance as a shared_ptr
		auto instance = std::make_shared<background_removal_filter>();

		instance->source = source;
		instance->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);

		std::string instanceName{"background-removal-inference"};
		instance->env.reset(new Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_ERROR, instanceName.c_str()));

		instance->modelSelection.clear();

		// Create pointer to shared_ptr for the update call
		auto ptr = new std::shared_ptr<background_removal_filter>(instance);
		background_filter_update(ptr, settings);
		proc_handler_add(
			obs_source_get_proc_handler(source),
			"void camera_status(out int status, out bool gpu, out bool fallback, out bool preparing)",
			[](void *data, calldata_t *params) {
				auto *state = static_cast<std::shared_ptr<background_removal_filter> *>(data);
				calldata_set_int(params, "status", (*state)->penguinStatus.load());
				calldata_set_bool(params, "gpu", (*state)->activeGPU.load());
				calldata_set_bool(params, "fallback", (*state)->cpuFallback.load());
				calldata_set_bool(params, "preparing", (*state)->gpuPreparing.load());
			},
			ptr);

		// Return the pointer to the shared_ptr
		// This keeps the reference count at least 1 until destroy is called
		return ptr;
	} catch (const std::exception &e) {
		obs_log(LOG_ERROR, "Failed to create background filter: %s", e.what());
		return nullptr;
	}
}

void background_filter_destroy(void *data)
{
	obs_log(LOG_INFO, "Background filter destroyed");

	// Cast back to shared_ptr pointer
	auto *ptr = static_cast<std::shared_ptr<background_removal_filter> *>(data);
	if (ptr) {
		if (*ptr) {
			// Mark as disabled to prevent further processing
			(*ptr)->isDisabled = true;

			// Perform cleanup
			obs_enter_graphics();
			gs_texrender_destroy((*ptr)->texrender);
			if ((*ptr)->stagesurface) {
				gs_stagesurface_destroy((*ptr)->stagesurface);
			}
			gs_effect_destroy((*ptr)->effect);
			gs_effect_destroy((*ptr)->kawaseBlurEffect);
			obs_leave_graphics();
		}
		// Delete the pointer to shared_ptr
		// This decrements the ref count. If no other threads hold a shared_ptr, the instance is deleted
		delete ptr;
	}
}

static void processImageForBackground(struct background_removal_filter *tf, const cv::Mat &imageBGRA,
				      cv::Mat &backgroundMask)
{
	cv::Mat outputImage;
	auto *rvm = tf->modelSelection == MODEL_RVM ? static_cast<ModelRVM *>(tf->model.get()) : nullptr;
	bool handStarted = false;
	auto startHand = [&] {
		if (!rvm || !tf->preserveHands) {
			return;
		}
		cv::Mat small, rgb, input;
		cv::resize(imageBGRA, small, cv::Size(320, 192));
		cv::cvtColor(small, rgb, cv::COLOR_BGRA2RGB);
		rgb.convertTo(input, CV_32F);
		rvm->StartHandRecovery(input);
		handStarted = true;
	};
	if (tf->gpuWorker && !tf->gpuWorker->Preparing() && !tf->gpuWorker->Failed()) {
		startHand();
	}
	const bool gpu = tf->gpuWorker && tf->gpuWorker->Infer(imageBGRA, outputImage);
	if (gpu && tf->preserveHands) {
		if (!handStarted) {
			startHand();
		}
		cv::Mat alpha;
		outputImage.convertTo(alpha, CV_32F, 1.0 / 255.0);
		rvm->FinishHandRecovery(alpha);
		alpha.convertTo(outputImage, CV_8U, 255.0);
	}
	if (!gpu) {
		if (tf->gpuWorker && tf->gpuWorker->Failed() && !tf->cpuFallback) {
			obs_log(LOG_INFO, "GPU processing unavailable: %s; using CPU.", tf->gpuWorker->Failure());
		}
		if (tf->activeGPU) {
			obs_log(LOG_INFO, "GPU worker stopped; background removal continues on CPU.");
			// Keep the already prepared CPU session. Reset only RVM's recurrent
			// state so a driver failure cannot trigger model compilation here.
			for (size_t index = 1; index < 5 && index < tf->inputTensorValues.size(); ++index) {
				std::fill(tf->inputTensorValues[index].begin(), tf->inputTensorValues[index].end(),
					  0.0f);
			}
		}
		tf->activeGPU = false;
		tf->gpuPreparing = tf->gpuWorker && tf->gpuWorker->Preparing();
		tf->cpuFallback = tf->useGPU != "cpu" && !tf->gpuPreparing;
		if (!runFilterModelInference(tf, imageBGRA, outputImage)) {
			return;
		}
	} else {
		if (!tf->activeGPU) {
			obs_log(LOG_INFO, "Background removal is now GPU accelerated.");
		}
		tf->activeGPU = true;
		tf->gpuPreparing = false;
		tf->cpuFallback = false;
	}
	// Assume outputImage is now a single channel, uint8 image with values between 0 and 255

	// If we have a threshold, apply it. Otherwise, just use the output image as the mask
	if (tf->enableThreshold) {
		// We need to make tf->threshold (float [0,1]) be in that range
		const uint8_t threshold_value = (uint8_t)(tf->threshold * 255.0f);
		backgroundMask = outputImage < threshold_value;
	} else {
		backgroundMask = 255 - outputImage;
	}
}

void background_filter_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	auto *ptr = static_cast<std::shared_ptr<background_removal_filter> *>(data);
	if (ptr && *ptr) {
		(*ptr)->maskPending = true;
	}
}

static void background_filter_process_current_frame(void *data)
{

	// Cast to shared_ptr pointer and create a local shared_ptr
	auto *ptr = static_cast<std::shared_ptr<background_removal_filter> *>(data);
	if (!ptr) {
		return;
	}

	// Create a local shared_ptr
	// This guarantees the object stays alive for the duration of this function scope
	// even if filter_destroy is called on the main thread
	std::shared_ptr<background_removal_filter> tf = *ptr;

	if (!tf || tf->isDisabled) {
		return;
	}

	if (!obs_source_enabled(tf->source)) {
		return;
	}

	if (!tf->model) {
		tf->penguinStatus = 3;
		tf->isDisabled = true;
		obs_log(LOG_ERROR, "Model is not initialized");
		return;
	}

	cv::Mat imageBGRA;
	{
		std::unique_lock<std::mutex> lock(tf->inputBGRALock, std::try_to_lock);
		if (!lock.owns_lock()) {
			// No data to process
			return;
		}
		if (tf->inputBGRA.empty()) {
			// No data to process
			return;
		}
		imageBGRA = tf->inputBGRA.clone();
	}

	if (tf->enableImageSimilarity) {
		if (!tf->lastImageBGRA.empty() && !imageBGRA.empty() && tf->lastImageBGRA.size() == imageBGRA.size()) {
			// calculate PSNR
			double psnr = cv::PSNR(tf->lastImageBGRA, imageBGRA);

			if (psnr > tf->imageSimilarityThreshold) {
				// The image is almost the same as the previous one. Skip processing.
				return;
			}
		}
		tf->lastImageBGRA = imageBGRA.clone();
	}

	if (tf->backgroundMask.empty()) {
		// First frame. Initialize the background mask.
		tf->backgroundMask = cv::Mat(imageBGRA.size(), CV_8UC1, cv::Scalar(255));
	}

	tf->maskEveryXFramesCount++;
	tf->maskEveryXFramesCount %= tf->maskEveryXFrames;

	try {
		if (tf->maskEveryXFramesCount != 0 && !tf->backgroundMask.empty()) {
			// We are skipping processing of the mask for this frame.
			// Get the background mask previously generated.
			; // Do nothing
		} else {
			cv::Mat backgroundMask;

			{
				std::unique_lock<std::mutex> lock(tf->modelMutex);
				// Process the image to find the mask.
				processImageForBackground(tf.get(), imageBGRA, backgroundMask);
			}

			if (backgroundMask.empty()) {
				// Do not leave a stale mask hiding the person after inference fails.
				tf->penguinStatus = 3;
				tf->isDisabled = true;
				obs_log(LOG_ERROR,
					"Background inference produced an empty mask; preserving the input.");
				return;
			}

			// Temporal smoothing
			if (tf->temporalSmoothFactor > 0.0 && tf->temporalSmoothFactor < 1.0 &&
			    !tf->lastBackgroundMask.empty() && tf->lastBackgroundMask.size() == backgroundMask.size()) {

				float temporalSmoothFactor = tf->temporalSmoothFactor;
				if (tf->enableThreshold) {
					// The temporal smooth factor can't be smaller than the threshold
					temporalSmoothFactor = std::max(temporalSmoothFactor, tf->threshold);
				}

				cv::addWeighted(backgroundMask, temporalSmoothFactor, tf->lastBackgroundMask,
						1.0 - temporalSmoothFactor, 0.0, backgroundMask);
			}

			tf->lastBackgroundMask = backgroundMask.clone();

			// Contour processing
			// Only applicable if we are thresholding (and get a binary image)
			if (tf->enableThreshold) {
				if (tf->contourFilter > 0.0 && tf->contourFilter < 1.0) {
					std::vector<std::vector<cv::Point>> contours;
					findContours(backgroundMask, contours, cv::RETR_EXTERNAL,
						     cv::CHAIN_APPROX_SIMPLE);
					std::vector<std::vector<cv::Point>> filteredContours;
					const double contourSizeThreshold =
						(double)(backgroundMask.total()) * tf->contourFilter;
					for (auto &contour : contours) {
						if (cv::contourArea(contour) > (double)contourSizeThreshold) {
							filteredContours.push_back(contour);
						}
					}
					backgroundMask.setTo(0);
					drawContours(backgroundMask, filteredContours, -1, cv::Scalar(255), -1);
				}

				if (tf->smoothContour > 0.0) {
					int k_size = (int)(3 + 11 * tf->smoothContour);
					k_size += k_size % 2 == 0 ? 1 : 0;
					cv::stackBlur(backgroundMask, backgroundMask, cv::Size(k_size, k_size));
				}

				// Resize the size of the mask back to the size of the original input.
				cv::resize(backgroundMask, backgroundMask, imageBGRA.size());

				// Additional contour processing at full resolution
				if (tf->smoothContour > 0.0) {
					// If the mask was smoothed, apply a threshold to get a binary mask
					backgroundMask = backgroundMask > 128;
				}

				// Expand or shrink the mask
				if (tf->maskExpansion > 0.0) {
					cv::erode(backgroundMask, backgroundMask, cv::Mat(), cv::Point(-1, -1),
						  tf->maskExpansion);
				} else if (tf->maskExpansion < 0.0) {
					cv::dilate(backgroundMask, backgroundMask, cv::Mat(), cv::Point(-1, -1),
						   -tf->maskExpansion);
				}

				if (tf->feather > 0.0) {
					// Feather (blur) the mask
					int k_size = (int)(40 * tf->feather);
					k_size += k_size % 2 == 0 ? 1 : 0;
					cv::dilate(backgroundMask, backgroundMask, cv::Mat(), cv::Point(-1, -1),
						   k_size / 3);
					cv::boxFilter(backgroundMask, backgroundMask, tf->backgroundMask.depth(),
						      cv::Size(k_size, k_size));
				}
			}

			// Save the mask for the next frame
			{
				std::lock_guard<std::mutex> lock(tf->outputLock);
				backgroundMask.copyTo(tf->backgroundMask);
				tf->penguinStatus = 2;
			}
		}
	} catch (const Ort::Exception &e) {
		tf->penguinStatus = 3;
		tf->isDisabled = true;
		obs_log(LOG_ERROR, "ONNXRuntime Exception: %s", e.what());
	} catch (const std::exception &e) {
		tf->penguinStatus = 3;
		tf->isDisabled = true;
		obs_log(LOG_ERROR, "%s", e.what());
	}
}

static gs_texture_t *blur_background(std::shared_ptr<background_removal_filter> tf, uint32_t width, uint32_t height,
				     gs_texture_t *alphaTexture)
{
	if (tf->blurBackground == 0 || !tf->kawaseBlurEffect) {
		return nullptr;
	}
	gs_texture_t *blurredTexture = gs_texture_create(width, height, GS_BGRA, 1, nullptr, 0);
	gs_copy_texture(blurredTexture, gs_texrender_get_texture(tf->texrender));
	gs_eparam_t *image = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "image");
	gs_eparam_t *focalmask = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "focalmask");
	gs_eparam_t *xOffset = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "xOffset");
	gs_eparam_t *yOffset = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "yOffset");
	gs_eparam_t *blurIter = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "blurIter");
	gs_eparam_t *blurTotal = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "blurTotal");
	gs_eparam_t *blurFocusPointParam = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "blurFocusPoint");
	gs_eparam_t *blurFocusDepthParam = gs_effect_get_param_by_name(tf->kawaseBlurEffect, "blurFocusDepth");

	for (int i = 0; i < (int)tf->blurBackground; i++) {
		gs_texrender_reset(tf->texrender);
		if (!gs_texrender_begin(tf->texrender, width, height)) {
			obs_log(LOG_INFO, "Could not open background blur texrender!");
			return blurredTexture;
		}

		gs_effect_set_texture(image, blurredTexture);
		gs_effect_set_texture(focalmask, alphaTexture);
		gs_effect_set_float(xOffset, ((float)i + 0.5f) / (float)width);
		gs_effect_set_float(yOffset, ((float)i + 0.5f) / (float)height);
		gs_effect_set_int(blurIter, i);
		gs_effect_set_int(blurTotal, (int)tf->blurBackground);
		gs_effect_set_float(blurFocusPointParam, tf->blurFocusPoint);
		gs_effect_set_float(blurFocusDepthParam, tf->blurFocusDepth);

		struct vec4 background;
		vec4_zero(&background);
		gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
		gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		const char *blur_type = (tf->enableFocalBlur) ? "DrawFocalBlur" : "Draw";

		while (gs_effect_loop(tf->kawaseBlurEffect, blur_type)) {
			gs_draw_sprite(blurredTexture, 0, width, height);
		}
		gs_blend_state_pop();
		gs_texrender_end(tf->texrender);
		gs_copy_texture(blurredTexture, gs_texrender_get_texture(tf->texrender));
	}
	return blurredTexture;
}

void background_filter_video_render(void *data, gs_effect_t *_effect)
{
	UNUSED_PARAMETER(_effect);

	// Cast to shared_ptr pointer and create a local shared_ptr
	auto *ptr = static_cast<std::shared_ptr<background_removal_filter> *>(data);
	if (!ptr) {
		return;
	}

	// Create a local shared_ptr
	std::shared_ptr<background_removal_filter> tf = *ptr;

	if (!tf || tf->isDisabled) {
		if (tf && tf->source) {
			obs_source_skip_video_filter(tf->source);
		}
		return;
	}

	uint32_t width, height;
	if (!getRGBAFromStageSurface(tf.get(), width, height)) {
		if (tf->source) {
			obs_source_skip_video_filter(tf->source);
		}
		return;
	}

	// The tick runs before rendering and therefore only has the previous input.
	// Infer after staging this frame so moving hands and heads use their own
	// mask. Multiple renders in one video tick must not advance recurrent state.
	if (tf->maskPending) {
		tf->maskPending = false;
		background_filter_process_current_frame(data);
	}
	if (tf->isDisabled) {
		obs_source_skip_video_filter(tf->source);
		return;
	}

	if (!tf->effect) {
		// Effect failed to load, skip rendering
		if (tf->source) {
			obs_source_skip_video_filter(tf->source);
		}
		return;
	}

	gs_texture_t *alphaTexture = nullptr;
	{
		std::lock_guard<std::mutex> lock(tf->outputLock);

		if (tf->backgroundMask.empty()) {
			// Preserve the input if no mask could be produced.
			// Inference failures are reported through camera_status.
			if (tf->source) {
				obs_source_skip_video_filter(tf->source);
			}
			return;
		}

		alphaTexture = gs_texture_create(tf->backgroundMask.cols, tf->backgroundMask.rows, GS_R8, 1,
						 (const uint8_t **)&tf->backgroundMask.data, 0);

		if (!alphaTexture) {
			obs_log(LOG_ERROR, "Failed to create alpha texture");
			if (tf->source) {
				obs_source_skip_video_filter(tf->source);
			}
			return;
		}
	}

	// Output the masked image
	gs_texture_t *blurredTexture = blur_background(tf, width, height, alphaTexture);

	if (!obs_source_process_filter_begin(tf->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
		if (tf->source) {
			obs_source_skip_video_filter(tf->source);
		}
		gs_texture_destroy(alphaTexture);
		gs_texture_destroy(blurredTexture);
		return;
	}

	gs_eparam_t *alphamask = gs_effect_get_param_by_name(tf->effect, "alphamask");
	gs_eparam_t *blurredBackground = gs_effect_get_param_by_name(tf->effect, "blurredBackground");

	gs_effect_set_texture(alphamask, alphaTexture);

	if (tf->blurBackground > 0) {
		gs_effect_set_texture(blurredBackground, blurredTexture);
	}

	gs_blend_state_push();
	gs_reset_blend_state();

	const char *techName;
	if (tf->blurBackground > 0) {
		if (tf->enableFocalBlur) {
			techName = "DrawWithFocalBlur";
		} else {
			techName = "DrawWithBlur";
		}
	} else {
		techName = "DrawWithoutBlur";
	}

	obs_source_process_filter_tech_end(tf->source, tf->effect, 0, 0, techName);

	gs_blend_state_pop();

	gs_texture_destroy(alphaTexture);
	gs_texture_destroy(blurredTexture);
}
