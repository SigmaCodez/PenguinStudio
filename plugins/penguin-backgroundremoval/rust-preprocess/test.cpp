// SPDX-License-Identifier: GPL-3.0-or-later
#include "preprocess.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

static void require(bool ok, const char *message)
{
	if (!ok)
		throw std::runtime_error(message);
}

// Original GPU worker preprocessing, including ModelBCHW and ModelRVM copies.
static void original(const cv::Mat &input, std::vector<float> &tensor)
{
	cv::Mat rgb, image, prepared;
	cv::cvtColor(input, rgb, cv::COLOR_BGRA2RGB);
	rgb.convertTo(image, CV_32F);
	image = image / 255.0;
	std::vector<cv::Mat> channels;
	cv::split(image, channels);
	for (auto &channel : channels)
		channel = channel.reshape(1, 1);
	cv::hconcat(channels, prepared);
	tensor.assign(prepared.begin<float>(), prepared.end<float>());
}

static void rust(const cv::Mat &input, std::vector<float> &tensor)
{
	const auto bytes = input.step[0] * (input.rows - 1) + size_t(input.cols) * 4;
	require(penguin_bgra_to_rgb_chw(input.data, bytes, input.cols, input.rows, input.step[0], tensor.data(),
				      tensor.size()) == 1, "Rust conversion rejected a valid frame");
}

// Control: the same fused algorithm in C++, to separate algorithmic gains from
// language/compiler differences. This is a benchmark reference, not production.
static void fusedCpp(const cv::Mat &input, std::vector<float> &tensor)
{
	const size_t plane = input.total();
	for (int y = 0; y < input.rows; ++y) {
		const auto *row = input.ptr<unsigned char>(y);
		for (int x = 0; x < input.cols; ++x) {
			const size_t i = size_t(y) * input.cols + x;
			tensor[i] = row[x * 4 + 2] * (1.0f / 255.0f);
			tensor[i + plane] = row[x * 4 + 1] * (1.0f / 255.0f);
			tensor[i + 2 * plane] = row[x * 4] * (1.0f / 255.0f);
		}
	}
}

static volatile float sink;
static double measure(void (*fn)(const cv::Mat &, std::vector<float> &), const cv::Mat &input,
		      std::vector<float> &tensor)
{
	for (int i = 0; i < 30; ++i)
		fn(input, tensor);
	const auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < 1000; ++i) {
		fn(input, tensor);
		sink = tensor[size_t(i) % tensor.size()];
	}
	return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / 1000;
}

int main(int argc, char **)
{
	try {
		cv::setNumThreads(1);
		cv::RNG rng(12345);
		float maxError = 0;
		for (const cv::Size size : {cv::Size(1, 1), cv::Size(256, 1), cv::Size(17, 9),
					  cv::Size(320, 192), cv::Size(1920, 1080)}) {
			for (int padding : {0, 7}) {
				cv::Mat storage(size.height + 2, size.width + padding, CV_8UC4);
				rng.fill(storage, cv::RNG::UNIFORM, 0, 256);
				auto input = storage(cv::Rect(0, 1, size.width, size.height));
				if (size.width == 256 && size.height == 1) {
					for (int x = 0; x < 256; ++x)
						input.at<cv::Vec4b>(0, x) = cv::Vec4b(x, 255 - x, (x + 71) % 256, x);
				}
				const auto saved = storage.clone();
				std::vector<float> reference, actual(input.total() * 3), fused(actual.size());
				original(input, reference);
				rust(input, actual);
				fusedCpp(input, fused);
				for (size_t i = 0; i < actual.size(); ++i) {
					maxError = std::max(maxError, std::abs(actual[i] - reference[i]));
					require(std::isfinite(actual[i]) && std::abs(actual[i] - reference[i]) <= 1e-7f,
						"Tensor mismatch with original preprocessing");
					require(actual[i] == fused[i], "Mismatch with fused C++ control");
				}
				require(cv::norm(storage, saved, cv::NORM_INF) == 0, "Input modified");
			}
		}
		std::cout << "10 tensor equivalence cases passed; maximum absolute error: " << maxError << '\n';
		if (argc > 1) {
			cv::Mat input(192, 320, CV_8UC4);
			rng.fill(input, cv::RNG::UNIFORM, 0, 256);
			std::vector<float> tensor(input.total() * 3);
			std::vector<double> baseline, candidate, control;
			for (int trial = 0; trial < 9; ++trial) {
				// Alternate order to reduce warm-up / thermal bias.
				if (trial % 2) {
					candidate.push_back(measure(rust, input, tensor));
					baseline.push_back(measure(original, input, tensor));
				} else {
					baseline.push_back(measure(original, input, tensor));
					candidate.push_back(measure(rust, input, tensor));
				}
				control.push_back(measure(fusedCpp, input, tensor));
			}
			for (auto *values : {&baseline, &candidate, &control})
				std::sort(values->begin(), values->end());
			std::cout << "Median microseconds/frame (320x192, 9 x 1000): OpenCV=" << baseline[4]
				  << " Rust=" << candidate[4] << " fused-C++=" << control[4]
				  << " preprocessing speedup=" << baseline[4] / candidate[4] << "x\n";
		}
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
