// SPDX-License-Identifier: GPL-3.0-or-later
#include "hand-softmax.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

static void require(bool ok, const char *message)
{
	if (!ok)
		throw std::runtime_error(message);
}

static float reference(const float *values, bool cache = false)
{
	const float maximum = *std::max_element(values, values + 6);
	float sum = 0, skin = 0;
	for (int channel = 0; channel < 6; ++channel) {
		const float value = std::exp(values[channel] - maximum);
		sum += value;
		if (cache && channel == 2)
			skin = value;
	}
	if (!cache)
		skin = std::exp(values[2] - maximum);
	const float alpha = std::clamp((skin / sum - .2f) / .45f, 0.f, 1.f);
	return alpha * alpha * (3 - 2 * alpha);
}

static void same(float actual, float expected)
{
	require((actual == expected && (actual != 0 || std::signbit(actual) == std::signbit(expected))) ||
			(std::isnan(actual) && std::isnan(expected)), "Skin alpha mismatch");
}

static void checkLayouts()
{
	std::mt19937 rng(129);
	for (size_t width : {1, 7, 31, 154, 256}) {
		for (size_t height : {1, 3, 154}) {
			for (size_t padding : {0, 13}) {
				const size_t stride = width * 6 + padding, targetStride = width + 3;
				const size_t inputSize = (height - 1) * stride + width * 6;
				const size_t targetSize = (height - 1) * targetStride + width;
				std::vector<float> input(inputSize + 1), target(targetSize + 2, -42.f);
				for (float &value : input)
					value = (int(rng() % 1601) - 800) / 100.f;
				const auto saved = input;
				require(penguin_logits6_to_skin_mask(input.data() + 1, inputSize, width, height,
							    stride, target.data() + 1, targetSize, targetStride) == 1,
					"Valid strided input rejected");
				for (size_t y = 0; y < height; ++y) {
					for (size_t x = 0; x < width; ++x)
						same(target[1 + y * targetStride + x], reference(input.data() + 1 + y * stride + x * 6));
					if (y + 1 < height)
						for (size_t x = width; x < targetStride; ++x)
							require(target[1 + y * targetStride + x] == -42.f, "Output padding modified");
				}
				require(target.front() == -42.f && target.back() == -42.f && input == saved,
					"Guard or source modified");
			}
		}
	}
	for (size_t channel = 0; channel < 6; ++channel) {
		for (float value : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
				   -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max(),
				   std::numeric_limits<float>::lowest(), std::numeric_limits<float>::denorm_min()}) {
			float values[6] = {0.1f, -0.5f, .7f, 1.f, 0.f, -1.f}, result;
			values[channel] = value;
			require(penguin_logits6_to_skin_mask(values, 6, 1, 1, 6, &result, 1, 1) == 1,
				"Nonfinite value rejected");
			same(result, reference(values));
		}
	}
	std::cout << "30 strided layouts and 36 nonfinite/extreme input cases match C++ exactly\n";
}

static void checkGuardPages()
{
#ifdef __linux__
	const auto page = size_t(sysconf(_SC_PAGESIZE));
	struct Mapping {
		unsigned char *data;
		size_t bytes;
		~Mapping() { munmap(data, bytes); }
	};
	auto allocate = [page]() {
		auto *data = static_cast<unsigned char *>(mmap(nullptr, page * 2, PROT_READ | PROT_WRITE,
							     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
		require(data != MAP_FAILED, "Guard allocation failed");
		if (mprotect(data + page, page, PROT_NONE) != 0) {
			munmap(data, page * 2);
			throw std::runtime_error("Guard protection failed");
		}
		return Mapping{data, page * 2};
	};
	auto source = allocate();
	auto target = allocate();
	for (size_t width = 1; width <= 65; ++width) {
		auto *input = reinterpret_cast<float *>(source.data + page - width * 6 * sizeof(float));
		auto *output = reinterpret_cast<float *>(target.data + page - width * sizeof(float));
		for (size_t i = 0; i < width * 6; ++i)
			input[i] = float(i % 17) / 3.f;
		require(penguin_logits6_to_skin_mask(input, width * 6, width, 1, width * 6, output, width, width) == 1,
			"Guarded conversion failed");
		for (size_t x = 0; x < width; ++x)
			same(output[x], reference(input + x * 6));
	}
	std::cout << "65 guarded hand-softmax boundary cases passed\n";
#endif
}

static void checkSaturationBoundaries()
{
	for (float base : {-1000.f, -32.f, 0.f, 32.f, 1000.f, 1.e20f}) {
		for (int competitor : {0, 1, 3, 4, 5}) {
			for (float threshold : {2.f, 3.f}) {
				for (int direction : {-1, 1}) {
					float gap = threshold;
					for (int step = 0; step <= 64; ++step) {
						float values[6], result;
						std::fill(values, values + 6, base - 5.f);
						values[2] = base;
						values[competitor] = base + gap;
						require(penguin_logits6_to_skin_mask(values, 6, 1, 1, 6, &result, 1, 1) == 1,
							"Background saturation case rejected");
						same(result, reference(values));
						std::fill(values, values + 6, base);
						values[2] = base + gap;
						require(penguin_logits6_to_skin_mask(values, 6, 1, 1, 6, &result, 1, 1) == 1,
							"Skin saturation case rejected");
						same(result, reference(values));
						gap = std::nextafter(gap, direction < 0 ? 0.f : 10.f);
					}
				}
			}
		}
	}
	std::cout << "15,600 saturation-boundary cases match the original C++ calculation\n";
}

static void checkBlockDispatch()
{
	for (size_t width : {1, 7, 63, 64, 65, 127, 128, 129, 256}) {
		for (int pattern = 0; pattern < 4; ++pattern) {
			std::vector<float> input(width * 6, 0.f), output(width);
			for (size_t x = 0; x < width; ++x) {
				// Saturated samples must never cause the interior of a block to
				// be approximated, including uncertain or nonfinite interior pixels.
				input[x * 6 + 2] = pattern == 0 || x % 64 == 0 || x % 64 == 32 || x % 64 == 63
							  ? -8.f : 0.5f;
				if (pattern == 2 && x % 2)
					input[x * 6 + 1] = std::numeric_limits<float>::quiet_NaN();
				if (pattern == 3 && x % 2)
					input[x * 6 + 2] = 8.f;
			}
			require(penguin_logits6_to_skin_mask(input.data(), input.size(), width, 1, width * 6,
						    output.data(), output.size(), width) == 1,
				"Block dispatch case rejected");
			for (size_t x = 0; x < width; ++x)
				same(output[x], reference(input.data() + x * 6));
		}
	}
	std::cout << "36 mixed block-boundary cases preserve every pixel\n";
}

static volatile float sink;
static void benchmark(const char *distribution)
{
	// Actual hand-model crop for the GPU worker's 320x192 frame: round(192*256/320)=154.
	constexpr size_t width = 256, height = 154, pixels = width * height;
	std::mt19937 rng(29);
	std::vector<float> input(pixels * 6), output(pixels);
	for (size_t pixel = 0; pixel < pixels; ++pixel) {
		for (size_t channel = 0; channel < 6; ++channel) {
			float value = (int(rng() % 1601) - 800) / 100.f;
			if (std::string(distribution) == "ambiguous")
				value *= 0.1f;
			else if (std::string(distribution) == "background")
				value = channel == 2 ? -8.f : value * 0.1f;
			else if (std::string(distribution) == "skin")
				value = channel == 2 ? 8.f : value * 0.1f;
			input[pixel * 6 + channel] = value;
		}
	}
	// Check all benchmark pixels, including distributions taking the exact
	// saturation paths, against the original before measuring any timing.
	require(penguin_logits6_to_skin_mask(input.data(), input.size(), width, height,
					    width * 6, output.data(), output.size(), width) == 1,
		"Benchmark verification conversion failed");
	for (size_t pixel = 0; pixel < pixels; ++pixel)
		same(output[pixel], reference(input.data() + pixel * 6));
	std::vector<double> times[3];
	for (int trial = 0; trial < 9; ++trial) {
		for (int step = 0; step < 3; ++step) {
			const int method = (trial + step) % 3;
			const auto start = std::chrono::steady_clock::now();
			for (int i = 0; i < 100; ++i) {
				if (method == 1) {
					require(penguin_logits6_to_skin_mask(input.data(), input.size(), width, height,
								    width * 6, output.data(), output.size(), width) == 1,
						"Benchmark conversion failed");
				} else {
					for (size_t x = 0; x < pixels; ++x)
						output[x] = reference(input.data() + x * 6, method == 2);
				}
				sink = output[size_t(i)];
			}
			times[method].push_back(std::chrono::duration<double, std::micro>(
				std::chrono::steady_clock::now() - start).count() / 100);
		}
	}
	for (auto &values : times)
		std::sort(values.begin(), values.end());
	std::cout << "Median microseconds/frame (256x154, 9x100, " << distribution << "): C++=" << times[0][4]
		  << " Rust=" << times[1][4] << " C++-cached-exp=" << times[2][4] << '\n';
}

int main(int argc, char **)
{
	try {
		checkLayouts();
		checkGuardPages();
		checkSaturationBoundaries();
		checkBlockDispatch();
		if (argc > 1)
			for (const char *distribution : {"mixed", "ambiguous", "background", "skin"})
				benchmark(distribution);
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
