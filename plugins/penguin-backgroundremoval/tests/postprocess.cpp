// SPDX-License-Identifier: GPL-3.0-or-later
// Standalone regression and informational benchmark for BCHW postprocessing.
// Compile with ORT_API_MANUAL_INIT and NDEBUG, matching release OpenCV behavior.
#include <obs-module.h>
#include "../vendor/src/models/Model.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>

extern "C" void obs_log(int, const char *, ...) {}

static void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static void original(cv::Mat &output)
{
	cv::Mat transposed;
	chw_to_hwc_32f(output, transposed);
	transposed.copyTo(output);
}

static void checkEquivalence()
{
	ModelBCHW model;
	cv::RNG random(4711);
	for (const int channels : {1, 2, 3, 6}) {
		for (const cv::Size size : {cv::Size(1, 1), cv::Size(17, 9), cv::Size(320, 192)}) {
			cv::Mat actual(size, CV_MAKETYPE(CV_32F, channels));
			random.fill(actual, cv::RNG::UNIFORM, -10., 10.);
			cv::Mat expected = actual.clone();
			auto *const storage = actual.data;
			original(expected);
			model.postprocessOutput(actual);
			require(actual.size() == expected.size() && actual.type() == expected.type(), "Shape changed");
			require(std::memcmp(actual.data, expected.data, actual.total() * actual.elemSize()) == 0,
				"Postprocessing changed tensor values");
			require(actual.data == storage, "Output storage changed");
		}
	}
	// Single-channel views need no layout conversion, even with padding. Retain
	// all bit patterns (including NaN, infinity and signed zero) and row guards.
	cv::Mat backing(5, 19, CV_32F);
	random.fill(backing, cv::RNG::UNIFORM, -10., 10.);
	const uint32_t special[] = {0x7fc01234, 0x7f800000, 0xff800000, 0x80000000};
	std::memcpy(backing.ptr<float>(1) + 1, special, sizeof(special));
	const auto saved = backing.clone();
	auto view = backing(cv::Rect(1, 1, 17, 3));
	auto *const storage = view.data;
	model.postprocessOutput(view);
	require(view.data == storage && !view.isContinuous(), "View layout changed");
	require(std::memcmp(backing.data, saved.data, backing.total() * backing.elemSize()) == 0,
		"Single-channel view or padding changed");
	std::cout << "12 tensor comparisons and special-value/padded-view case passed\n";
}

static volatile float sink;
static double measure(bool optimized)
{
	ModelBCHW model;
	cv::Mat output(192, 320, CV_32F, cv::Scalar(.5f));
	const auto start = std::chrono::steady_clock::now();
	for (int frame = 0; frame < 10000; ++frame) {
		if (optimized)
			model.postprocessOutput(output);
		else
			original(output);
		sink = output.ptr<float>()[frame % output.total()];
	}
	return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / 10000;
}

int main()
{
	try {
		cv::setNumThreads(1);
		checkEquivalence();
		std::vector<double> before, after;
		for (int trial = 0; trial < 9; ++trial) {
			if (trial % 2) {
				after.push_back(measure(true));
				before.push_back(measure(false));
			} else {
				before.push_back(measure(false));
				after.push_back(measure(true));
			}
		}
		std::sort(before.begin(), before.end());
		std::sort(after.begin(), after.end());
		std::cout << "Median microseconds/frame (320x192 single channel, 9 x 10000): previous="
			  << before[4] << " optimized=" << after[4] << '\n';
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
