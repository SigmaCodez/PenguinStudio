// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <memory>
#include <filesystem>
#include <obs-module.h>
#include <string>
#include <onnxruntime_cxx_api.h>

namespace PenguinInference {
struct DeviceState {
	bool gpu = false;
	bool fallback = false;
};

inline std::unique_ptr<Ort::Session> CreateSession(Ort::Env &env, const ORTCHAR_T *path, const std::string &mode,
						   int threads, DeviceState &state, const ORTCHAR_T *gpuPath = nullptr,
						   const std::string &cache = {})
{
	auto options = [threads] {
		Ort::SessionOptions result;
		result.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
		result.SetInterOpNumThreads(1);
		result.SetIntraOpNumThreads(std::max(1, threads));
		result.AddConfigEntry("session.intra_op.allow_spinning", "0");
		result.AddConfigEntry("session.inter_op.allow_spinning", "0");
		return result;
	};
	state = {};
	if (mode != "cpu") {
		const auto available = Ort::GetAvailableProviders();
		for (const auto *provider : {"MIGraphXExecutionProvider", "CUDAExecutionProvider"}) {
			if (std::find(available.begin(), available.end(), provider) == available.end()) {
				continue;
			}
			try {
				auto gpuOptions = options();
				gpuOptions.DisableMemPattern();
				gpuOptions.SetExecutionMode(ORT_SEQUENTIAL);
				if (std::string(provider) == "MIGraphXExecutionProvider") {
					std::unordered_map<std::string, std::string> settings{{"device_id", "0"},
											      {"migraphx_mem_limit",
											       "2147483648"}};
					if (!cache.empty()) {
						std::error_code error;
						std::filesystem::create_directories(cache, error);
						if (!error) {
							settings.emplace("migraphx_model_cache_dir", cache);
						}
					}
					gpuOptions.AppendExecutionProvider("MIGraphX", settings);
				} else {
					OrtCUDAProviderOptions config{};
					config.gpu_mem_limit = size_t(2) * 1024 * 1024 * 1024;
					gpuOptions.AppendExecutionProvider_CUDA(config);
				}
				auto session =
					std::make_unique<Ort::Session>(env, gpuPath ? gpuPath : path, gpuOptions);
				state.gpu = true;
				return session;
			} catch (const Ort::Exception &) {
				// A missing driver or an unsupported model must not disable the camera.
			}
		}
		state.fallback = true;
	}
	auto cpuOptions = options();
	return std::make_unique<Ort::Session>(env, path, cpuOptions);
}
} // namespace PenguinInference
