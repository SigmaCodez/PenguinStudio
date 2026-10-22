// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <onnxruntime_cxx_api.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace PenguinCamera {
// The pinned MediaPipe portrait model takes NHWC RGB in [0, 1].
class Segmenter {
	Ort::Env environment{ORT_LOGGING_LEVEL_ERROR, "PenguinCamera"};
	Ort::SessionOptions options;
	Ort::Session session{nullptr};
	std::string inputName, outputName;
	std::vector<float> input;

public:
	static constexpr uint32_t Width = 256, Height = 256;
	explicit Segmenter(const char *path)
	{
		options.SetIntraOpNumThreads(2);
		options.SetInterOpNumThreads(1);
		options.AddConfigEntry("session.intra_op.allow_spinning", "0");
		session = Ort::Session(environment, path, options);
		if (session.GetInputCount() != 1 || session.GetOutputCount() != 1 ||
		    session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape() !=
			    std::vector<int64_t>{1, Height, Width, 3} ||
		    session.GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape() !=
			    std::vector<int64_t>{1, Height, Width, 1})
			throw std::runtime_error("Unexpected portrait model tensor layout");
		Ort::AllocatorWithDefaultOptions allocator;
		inputName = session.GetInputNameAllocated(0, allocator).get();
		outputName = session.GetOutputNameAllocated(0, allocator).get();
		input.resize(Width * Height * 3);
	}
	std::vector<uint8_t> Process(const std::vector<uint8_t> &rgba)
	{
		if (rgba.size() != Width * Height * 4)
			throw std::runtime_error("Invalid camera frame size");
		for (size_t pixel = 0; pixel < Width * Height; ++pixel)
			for (size_t channel = 0; channel < 3; ++channel)
				input[pixel * 3 + channel] = rgba[pixel * 4 + channel] / 255.0f;
		auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
		const std::array<int64_t, 4> shape{1, Height, Width, 3};
		auto tensor = Ort::Value::CreateTensor<float>(memory, input.data(), input.size(), shape.data(), shape.size());
		const char *inputs[] = {inputName.c_str()}, *outputs[] = {outputName.c_str()};
		auto result = session.Run(Ort::RunOptions{nullptr}, inputs, &tensor, 1, outputs, 1);
		const float *values = result.front().GetTensorData<float>();
		std::vector<uint8_t> mask(Width * Height);
		for (size_t i = 0; i < mask.size(); ++i) {
			const float value = values[i];
			if (!std::isfinite(value))
				throw std::runtime_error("Non-finite portrait mask");
			mask[i] = uint8_t(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
		}
		return mask;
	}
};
} // namespace PenguinCamera
