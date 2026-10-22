// SPDX-License-Identifier: GPL-2.0-or-later
#include "plugins/penguin-camera/segmenter.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc, char **argv)
{
	if (argc != 2 && argc != 4) {
		return 2;
	}
	try {
		PenguinCamera::Segmenter model(argv[1]);
		std::vector<uint8_t> frame(PenguinCamera::Segmenter::Width * PenguinCamera::Segmenter::Height * 4, 0);
		if (argc == 4) {
			std::ifstream file(argv[2], std::ios::binary);
			frame.assign(std::istreambuf_iterator<char>(file), {});
		}
		const auto mask = model.Process(frame);
		if (mask.size() != PenguinCamera::Segmenter::Width * PenguinCamera::Segmenter::Height) {
			return 3;
		}
		if (argc == 4) {
			std::ofstream file(argv[3], std::ios::binary);
			file << "P5\n256 256\n255\n";
			file.write(reinterpret_cast<const char *>(mask.data()), mask.size());
		}
		bool rejected = false;
		try {
			model.Process({});
		} catch (const std::runtime_error &) {
			rejected = true;
		}
		if (!rejected) {
			return 4;
		}
		std::cout << "Local portrait inference and invalid-frame rejection passed\n";
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
