// SPDX-FileCopyrightText: 2021-2026 Roy Shilkrot <roy.shil@gmail.com>
// SPDX-FileCopyrightText: 2023-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MODELRVM_H
#define MODELRVM_H

#include "Model.hpp"
#include <future>
#include "../../../single-task-worker.hpp"
#include "HandRecovery.hpp"
#ifdef PENGUIN_RUST_PREPROCESS
#include "preprocess.h"
#endif

class ModelRVM : public ModelBCHW {
private:
	bool preserveHands;
	int handThreads;
	std::string deviceMode;
	std::string handPath, cachePath;
	std::unique_ptr<HandRecovery> handRecovery;
	std::future<void> handTask;
	// Declared last so the worker joins before the model and buffers are destroyed.
	std::unique_ptr<PenguinSingleTaskWorker> handWorker;

public:
	explicit ModelRVM(bool preserve = false, int threads = 3, std::string mode = "cpu", std::string hand = {},
			  std::string cache = {})
		: preserveHands(preserve),
		  handThreads(threads),
		  deviceMode(std::move(mode)),
		  handPath(std::move(hand)),
		  cachePath(std::move(cache))
	{
	}
	~ModelRVM() {}

	void StartHandRecovery(const cv::Mat &image)
	{
		if (preserveHands) {
			// A failed primary inference may skip postprocessing. Join its companion
			// before reusing the hand model and its buffers for another frame.
			if (handTask.valid()) {
				handTask.get();
			}
			// The current MIGraphX hand graph differs materially from CPU output.
			// Keep this companion on CPU; primary RVM remains GPU accelerated.
			if (!handRecovery) {
				handRecovery = std::make_unique<HandRecovery>(handThreads, "cpu", handPath, cachePath);
			}
			if (!handWorker) handWorker = std::make_unique<PenguinSingleTaskWorker>();
			handTask = handWorker->submit([this, frame = image.clone()] { handRecovery->infer(frame); });
		}
	}

	bool prepareBGRAInput(const cv::Mat &image, uint32_t width, uint32_t height,
			      std::vector<std::vector<float>> &tensors) override
	{
#ifdef PENGUIN_RUST_PREPROCESS
		if (image.type() != CV_8UC4 || image.cols != int(width) || image.rows != int(height) ||
		    tensors.size() < 6 || tensors[5].empty())
			return false;
		const size_t bytes = image.step[0] * size_t(image.rows - 1) + size_t(image.cols) * 4;
		if (penguin_bgra_to_rgb_chw(image.data, bytes, width, height, image.step[0],
					  tensors[0].data(), tensors[0].size()) != 1)
			return false;
		// Hand recovery needs interleaved, unnormalized RGB from the same frame.
		if (preserveHands) {
			cv::Mat rgb, input;
			cv::cvtColor(image, rgb, cv::COLOR_BGRA2RGB);
			rgb.convertTo(input, CV_32F);
			StartHandRecovery(input);
		}
		tensors[5][0] = 1.0f;
		return true;
#else
		return false;
#endif
	}

	virtual void prepareInputToNetwork(cv::Mat &image, cv::Mat &prepared) override
	{
		StartHandRecovery(image);
		ModelBCHW::prepareInputToNetwork(image, prepared);
	}

	virtual void postprocessOutput(cv::Mat &output) override
	{
		ModelBCHW::postprocessOutput(output);
		FinishHandRecovery(output);
	}

	void FinishHandRecovery(cv::Mat &output)
	{
		if (handRecovery && handTask.valid()) {
			handTask.get();
			handRecovery->merge(output);
		}
	}

	virtual void populateInputOutputNames(const std::unique_ptr<Ort::Session> &session,
					      std::vector<Ort::AllocatedStringPtr> &inputNames,
					      std::vector<Ort::AllocatedStringPtr> &outputNames)
	{
		Ort::AllocatorWithDefaultOptions allocator;

		inputNames.clear();
		outputNames.clear();

		for (size_t i = 0; i < session->GetInputCount(); i++) {
			inputNames.push_back(session->GetInputNameAllocated(i, allocator));
		}
		for (size_t i = 1; i < session->GetOutputCount(); i++) {
			outputNames.push_back(session->GetOutputNameAllocated(i, allocator));
		}
	}

	virtual bool populateInputOutputShapes(const std::unique_ptr<Ort::Session> &session,
					       std::vector<std::vector<int64_t>> &inputDims,
					       std::vector<std::vector<int64_t>> &outputDims)
	{
		// Assuming model only has one input and one output image

		inputDims.clear();
		outputDims.clear();

		for (size_t i = 0; i < session->GetInputCount(); i++) {
			// Get input shape
			const Ort::TypeInfo inputTypeInfo = session->GetInputTypeInfo(i);
			const auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
			inputDims.push_back(inputTensorInfo.GetShape());
		}

		for (size_t i = 1; i < session->GetOutputCount(); i++) {
			// Get output shape
			const Ort::TypeInfo outputTypeInfo = session->GetOutputTypeInfo(i);
			const auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
			outputDims.push_back(outputTensorInfo.GetShape());
		}

		const int base_width = 320;
		const int base_height = 192;

		inputDims[0][0] = 1;
		inputDims[0][2] = base_height;
		inputDims[0][3] = base_width;
		for (size_t i = 1; i < 5; i++) {
			inputDims[i][0] = 1;
			inputDims[i][1] = (i == 1) ? 16 : (i == 2) ? 20 : (i == 3) ? 40 : 64;
			inputDims[i][2] = base_height / (2 << (i - 1));
			inputDims[i][3] = base_width / (2 << (i - 1));
		}

		outputDims[0][0] = 1;
		outputDims[0][2] = base_height;
		outputDims[0][3] = base_width;
		for (size_t i = 1; i < 5; i++) {
			outputDims[i][0] = 1;
			outputDims[i][2] = base_height / (2 << (i - 1));
			outputDims[i][3] = base_width / (2 << (i - 1));
		}
		return true;
	}

	virtual void loadInputToTensor(const cv::Mat &preprocessedImage, uint32_t, uint32_t,
				       std::vector<std::vector<float>> &inputTensorValues)
	{
		inputTensorValues[0].assign(preprocessedImage.begin<float>(), preprocessedImage.end<float>());
		inputTensorValues[5][0] = 1.0f;
	}

	virtual void assignOutputToInput(std::vector<std::vector<float>> &outputTensorValues,
					 std::vector<std::vector<float>> &inputTensorValues)
	{
		for (size_t i = 1; i < 5; i++) {
			inputTensorValues[i].assign(outputTensorValues[i].begin(), outputTensorValues[i].end());
		}
	}
};

#endif /* MODELRVM_H */
