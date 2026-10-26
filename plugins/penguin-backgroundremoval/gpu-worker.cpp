// SPDX-License-Identifier: GPL-3.0-or-later
#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <cerrno>
#include <csignal>
#include <sched.h>
#include <filesystem>
#include <iostream>
#include <obs-module.h>
#ifdef PENGUIN_RUST_PREPROCESS
#include "rust-preprocess/preprocess.h"
#endif
#include "vendor/src/models/ModelRVM.hpp"
#include "vendor/src/ort-utils/ORTModelData.hpp"

// Model adapters only use the module lookup for their default paths. The worker
// always receives explicit package paths and never reads user scene settings.
extern "C" obs_module_t *obs_current_module(void)
{
	return nullptr;
}

static bool Transfer(void *buffer, size_t size, bool write)
{
	auto *bytes = static_cast<unsigned char *>(buffer);
	while (size) {
		const auto count = write ? send(3, bytes, size, MSG_NOSIGNAL) : recv(3, bytes, size, 0);
		if (count < 0 && errno == EINTR) {
			continue;
		}
		if (count <= 0) {
			return false;
		}
		bytes += count;
		size -= size_t(count);
	}
	return true;
}

int main(int argc, char **argv)
{
	if (argc != 6) {
		return 2;
	}
	const pid_t parent = getppid();
	prctl(PR_SET_PDEATHSIG, SIGKILL);
	if (parent == 1 || getppid() != parent) {
		return 2;
	}
	// Bound compiler concurrency while CPU matting keeps serving the preview.
	setenv("MIGRAPHX_COMPILE_PARALLEL", "1", 0);
	cpu_set_t original{}, compiling{};
	const bool affinity = sched_getaffinity(0, sizeof(original), &original) == 0;
	if (affinity) {
		int count = 0;
		for (int cpu = CPU_SETSIZE - 1; cpu >= 0 && count < 2; --cpu) {
			if (CPU_ISSET(cpu, &original)) {
				CPU_SET(cpu, &compiling);
				++count;
			}
		}
		if (count) {
			sched_setaffinity(0, sizeof(compiling), &compiling);
		}
	}
	try {
		void *runtime = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
		if (!runtime) {
			return 3;
		}
		using GetBase = const OrtApiBase *(*)();
		auto getBase = reinterpret_cast<GetBase>(dlsym(runtime, "OrtGetApiBase"));
		auto *api = getBase ? getBase()->GetApi(ORT_API_VERSION) : nullptr;
		if (!api) {
			return 3;
		}
		Ort::InitApi(api);
		const auto models = std::filesystem::path(argv[2]);
		const auto cache = std::filesystem::path(argv[3]);
		for (const auto &[variable, subdirectory] :
		     {std::pair{"TMPDIR", "tmp"}, std::pair{"AMD_COMGR_CACHE_DIR", "comgr"},
		      std::pair{"MIOPEN_USER_DB_PATH", "miopen-db"},
		      std::pair{"MIOPEN_CUSTOM_CACHE_DIR", "miopen-kernels"}}) {
			const auto directory = cache / subdirectory;
			std::filesystem::create_directories(directory);
			setenv(variable, directory.c_str(), 1);
		}
		const int threads = std::clamp(std::stoi(argv[4]), 1, 3);
		ModelRVM model(std::string(argv[5]) == "1", threads, "gpu",
			       (models / "selfie_multiclass_256x256.with_runtime_opt.ort").string(), argv[3]);
		ORTModelData data;
		data.env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_ERROR, "Penguin GPU worker");
		data.env->DisableTelemetryEvents();
		PenguinInference::DeviceState device;
		const auto path = models / "rvm_mobilenetv3_gpu.onnx";
		data.session = PenguinInference::CreateSession(*data.env, path.c_str(), "gpu", threads, device, nullptr,
							       argv[3]);
		if (!device.gpu) {
			return 4;
		}
		model.populateInputOutputNames(data.session, data.inputNames, data.outputNames);
		if (!model.populateInputOutputShapes(data.session, data.inputDims, data.outputDims)) {
			return 4;
		}
		model.allocateTensorBuffers(data.inputDims, data.outputDims, data.outputTensorValues,
					    data.inputTensorValues, data.inputTensor, data.outputTensor);
		cv::Mat input(192, 320, CV_8UC4, cv::Scalar(0, 0, 0, 255)), mask;
		auto infer = [&] {
#ifdef PENGUIN_RUST_PREPROCESS
			// Write into the already allocated ORT input tensor. Preserve the
			// original path on validation failure; model state is unchanged.
			auto &tensor = data.inputTensorValues[0];
			const bool converted = penguin_bgra_to_rgb_chw(
				input.data, input.step[0] * size_t(input.rows), size_t(input.cols),
				size_t(input.rows), input.step[0], tensor.data(), tensor.size()) == 1;
			if (converted) {
				data.inputTensorValues[5][0] = 1.0f;
			} else
#endif
			{
				cv::Mat rgb, image, prepared;
				cv::cvtColor(input, rgb, cv::COLOR_BGRA2RGB);
				rgb.convertTo(image, CV_32F);
				model.prepareInputToNetwork(image, prepared);
				model.loadInputToTensor(prepared, 320, 192, data.inputTensorValues);
			}
			model.runNetworkInference(data.session, data.inputNames, data.outputNames, data.inputTensor,
						  data.outputTensor);
			auto output = model.getNetworkOutput(data.outputDims, data.outputTensorValues);
			model.assignOutputToInput(data.outputTensorValues, data.inputTensorValues);
			model.postprocessOutput(output);
			output.convertTo(mask, CV_8U, 255.0);
		};
		for (int i = 0; i < 3; ++i) {
			infer();
		}
		if (affinity) {
			// Runtime threads inherit compilation affinity. Restore every thread
			// before serving frames, including the CPU hand companion.
			for (const auto &task : std::filesystem::directory_iterator("/proc/self/task")) {
				const auto id = static_cast<pid_t>(std::stoi(task.path().filename().string()));
				sched_setaffinity(id, sizeof(original), &original);
			}
		}
		unsigned char ready = 1;
		if (!Transfer(&ready, 1, true)) {
			return 0;
		}
		while (Transfer(input.data, 320 * 192 * 4, false)) {
			infer();
			if (!Transfer(mask.data, 320 * 192, true)) {
				break;
			}
		}
		data.session.reset();
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "Penguin GPU worker: " << error.what() << '\n';
		return 5;
	}
}
