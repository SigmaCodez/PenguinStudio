// SPDX-License-Identifier: GPL-3.0-or-later
// Native OpenCV/ORT-header regression for the CPU RVM Rust fast path.
#include <obs-module.h>
#include "../vendor/src/models/ModelRVM.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
extern "C" void obs_log(int, const char *, ...) {}
extern "C" obs_module_t *obs_current_module() { return nullptr; }
static void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }

static void original(ModelRVM &model, const cv::Mat &bgra, std::vector<std::vector<float>> &tensor)
{
	cv::Mat rgb, resized, image, prepared;
	cv::cvtColor(bgra, rgb, cv::COLOR_BGRA2RGB);
	cv::resize(rgb, resized, {320, 192});
	resized.convertTo(image, CV_32F);
	model.prepareInputToNetwork(image, prepared);
	model.loadInputToTensor(prepared, 320, 192, tensor);
}
static void candidate(ModelRVM &model, const cv::Mat &bgra, std::vector<std::vector<float>> &tensor)
{
	cv::Mat resized;
	if (bgra.size() != cv::Size(320, 192))
		cv::resize(bgra, resized, {320, 192});
	require(model.prepareBGRAInput(resized.empty() ? bgra : resized, 320, 192, tensor), "Fast path rejected input");
}
static std::vector<std::vector<float>> tensors()
{
	std::vector<std::vector<float>> t(6);
	t[0].resize(320 * 192 * 3);
	t[5].resize(1);
	return t;
}
int main()
{
	try {
		cv::setNumThreads(1);
		ModelRVM model;
		cv::RNG random(47289);
		int cases = 0;
		for (auto size : {cv::Size(1, 1), cv::Size(1, 193), cv::Size(323, 1), cv::Size(17, 9),
			cv::Size(320, 192), cv::Size(641, 479), cv::Size(1280, 720), cv::Size(1920, 1080)}) {
			for (int trial = 0; trial < 12; ++trial) {
				cv::Mat storage(size.height + 2, size.width + 6, CV_8UC4);
				random.fill(storage, cv::RNG::UNIFORM, 0, 256);
				auto input = storage(cv::Rect(2, 1, size.width, size.height));
				auto a = tensors(), b = tensors();
				original(model, input, a);
				candidate(model, input, b);
				require(std::memcmp(a[0].data(), b[0].data(), a[0].size() * sizeof(float)) == 0, "Tensor mismatch");
				require(a[5] == b[5], "Downsample ratio changed");
				// The hand companion must receive exactly the previous RGB pixels.
				cv::Mat oldRGB, oldSmall, smallBGRA, newRGB;
				cv::cvtColor(input, oldRGB, cv::COLOR_BGRA2RGB);
				cv::resize(oldRGB, oldSmall, {320, 192});
				cv::resize(input, smallBGRA, {320, 192});
				cv::cvtColor(smallBGRA, newRGB, cv::COLOR_BGRA2RGB);
				require(cv::norm(oldSmall, newRGB, cv::NORM_INF) == 0, "Hand pixels changed");
				++cases;
			}
		}
		auto invalid = tensors();
		cv::Mat wrong(10, 10, CV_8UC4);
		require(!model.prepareBGRAInput(wrong, 320, 192, invalid), "Wrong shape accepted");
		cv::Mat valid(192, 320, CV_8UC4, cv::Scalar(10, 20, 30, 255));
		invalid[0].resize(12);
		require(!model.prepareBGRAInput(valid, 320, 192, invalid), "Short tensor accepted");
		std::cout << cases << " exact tensor/hand-image comparisons and rejection cases passed\n";
		for (auto size : {cv::Size(320, 192), cv::Size(1920, 1080)}) {
			cv::Mat input(size, CV_8UC4); random.fill(input, cv::RNG::UNIFORM, 0, 256);
			std::vector<double> timings[2]; auto t = tensors();
			for (int trial = 0; trial < 9; ++trial) for (int mode : {trial % 2, 1 - trial % 2}) {
				auto begin = std::chrono::steady_clock::now();
				for (int i = 0; i < 400; ++i) {
					if (mode) candidate(model, input, t); else original(model, input, t);
				}
				timings[mode].push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now()-begin).count()/400);
			}
			for (int mode = 0; mode < 2; ++mode) std::sort(timings[mode].begin(),timings[mode].end());
			std::cout << size << " median us/frame: previous=" << timings[0][4] << " Rust=" << timings[1][4] << '\n';
		}
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
