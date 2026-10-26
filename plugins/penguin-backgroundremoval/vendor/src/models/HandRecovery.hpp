// SPDX-License-Identifier: GPL-3.0-or-later
// Same-frame skin recovery for the optional moving-hand detail mode.
#pragma once
#ifdef PENGUIN_RUST_PREPROCESS
#include "hand-softmax.h"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <obs-module.h>
#include <onnxruntime_cxx_api.h>
#include <opencv2/imgproc.hpp>
#include "HandRecoveryMask.hpp"
#include "../ort-utils/InferenceSession.hpp"

class HandRecovery {
	Ort::Env env{ORT_LOGGING_LEVEL_ERROR, "Penguin hand recovery"};
	std::unique_ptr<Ort::Session> session;
	std::vector<float> input;
	std::vector<float> output;
	cv::Mat mask;
	std::string filename;
	int threads;
	PenguinInference::DeviceState device;

public:
	explicit HandRecovery(int threads, const std::string &mode, const std::string &modelPath = {},
			      const std::string &cache = {})
		: threads(threads)
	{
		env.DisableTelemetryEvents();
		if (modelPath.empty()) {
			char *path = obs_module_file("models/selfie_multiclass_256x256.with_runtime_opt.ort");
			if (!path) {
				throw std::runtime_error("Hand recovery model missing");
			}
			filename = path;
			bfree(path);
		} else {
			filename = modelPath;
		}

		session = PenguinInference::CreateSession(env, filename.c_str(), mode, threads, device, nullptr, cache);
		input.resize(256 * 256 * 3);
		output.resize(256 * 256 * 6);
	}

	void infer(const cv::Mat &rgb)
	{
		const float scale = 256.f / std::max(rgb.cols, rgb.rows);
		const int width = std::max(1, int(std::round(rgb.cols * scale)));
		const int height = std::max(1, int(std::round(rgb.rows * scale)));
		cv::Mat padded(256, 256, CV_32FC3, input.data());
		padded.setTo(0);
		cv::Mat resized;
		cv::resize(rgb, resized, cv::Size(width, height), 0, 0, cv::INTER_AREA);
		resized.convertTo(padded(cv::Rect(0, 0, width, height)), CV_32FC3, 1. / 255.);

		const std::array<int64_t, 4> inputDims{1, 256, 256, 3};
		const std::array<int64_t, 4> outputDims{1, 256, 256, 6};
		auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
		auto in = Ort::Value::CreateTensor<float>(memory, input.data(), input.size(), inputDims.data(), 4);
		auto out = Ort::Value::CreateTensor<float>(memory, output.data(), output.size(), outputDims.data(), 4);
		Ort::AllocatorWithDefaultOptions allocator;
		auto inputName = session->GetInputNameAllocated(0, allocator);
		auto outputName = session->GetOutputNameAllocated(0, allocator);
		const char *inputs[] = {inputName.get()};
		const char *outputs[] = {outputName.get()};
		try {
			session->Run(Ort::RunOptions{nullptr}, inputs, &in, 1, outputs, &out, 1);
		} catch (const Ort::Exception &) {
			if (!device.gpu) {
				throw;
			}
			session = PenguinInference::CreateSession(env, filename.c_str(), "cpu", threads, device);
			device.fallback = true;
			blog(LOG_INFO, "[Penguin camera] Hand inference switched to CPU; retrying the same frame.");
			session->Run(Ort::RunOptions{nullptr}, inputs, &in, 1, outputs, &out, 1);
		}

		cv::Mat small(height, width, CV_32FC1);
		bool converted = false;
#ifdef PENGUIN_RUST_PREPROCESS
		converted = penguin_logits6_to_skin_mask(
			output.data(), output.size(), size_t(width), size_t(height), 256 * 6,
			small.ptr<float>(), small.total(), small.step[0] / sizeof(float)) == 1;
#endif
		if (!converted) {
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					// The model returns logits, not normalized class probabilities.
					const float *values = output.data() + (y * 256 + x) * 6;
					const float maximum = *std::max_element(values, values + 6);
					float sum = 0;
					for (int channel = 0; channel < 6; ++channel) {
						sum += std::exp(values[channel] - maximum);
					}
					const float skin = std::exp(values[2] - maximum) / sum;
					const float alpha = std::clamp((skin - .2f) / .45f, 0.f, 1.f);
					small.at<float>(y, x) = alpha * alpha * (3 - 2 * alpha);
				}
			}
		}
		cv::resize(small, mask, cv::Size(rgb.cols, rgb.rows));
	}

	void merge(cv::Mat &alpha) { MergeSupportedHandRecovery(alpha, mask); }
};
