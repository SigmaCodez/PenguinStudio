// SPDX-License-Identifier: GPL-3.0-or-later
#include "preprocess.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

static void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
	try {
		bool avx2 = false;
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
		__builtin_cpu_init();
		avx2 = __builtin_cpu_supports("avx2");
#endif
		if (argc == 2 && std::strcmp(argv[1], "--require-no-avx2") == 0)
			require(!avx2, "Test CPU unexpectedly exposes AVX2; fallback was not exercised");
		else if (argc == 2 && std::strcmp(argv[1], "--require-avx2") == 0)
			require(avx2, "Test CPU does not expose AVX2; SIMD path was not exercised");
		else
			require(argc == 1, "Unknown dispatch-test argument");

		for (size_t width = 1; width <= 65; ++width) {
			for (size_t padding : {0, 1, 7, 31}) {
				const size_t height = 3, stride = width * 4 + padding;
				const size_t bytes = (height - 1) * stride + width * 4;
				const size_t plane = width * height;
				// An offset input also exercises unaligned SIMD loads.
				std::vector<unsigned char> storage(bytes + 1);
				for (size_t i = 0; i < storage.size(); ++i)
					storage[i] = static_cast<unsigned char>(i * 73);
				const auto saved = storage;
				const auto *input = storage.data() + 1;
				std::vector<float> guarded(plane * 3 + 2, -42.0f);
				auto *output = guarded.data() + 1;
				require(penguin_bgra_to_rgb_chw(input, bytes, width, height, stride, output, plane * 3) == 1,
					"Conversion rejected a valid frame");
				for (size_t y = 0; y < height; ++y) {
					for (size_t x = 0; x < width; ++x) {
						const size_t i = y * width + x, source = y * stride + x * 4;
						require(output[i] == input[source + 2] * (1.0f / 255.0f) &&
							output[plane + i] == input[source + 1] * (1.0f / 255.0f) &&
							output[2 * plane + i] == input[source] * (1.0f / 255.0f),
							"Dispatch tensor mismatch");
					}
				}
				require(guarded.front() == -42.0f && guarded.back() == -42.0f, "Output guard overwritten");
				require(storage == saved, "Input modified");
			}
		}
		std::cout << "260 dispatch cases passed; CPU AVX2=" << avx2 << '\n';
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
