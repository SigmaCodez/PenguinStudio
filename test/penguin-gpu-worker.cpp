// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/penguin-backgroundremoval/gpu-worker-client.hpp"
#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <iostream>
#include <thread>

static bool Read(void *buffer, size_t size)
{
	auto *p = static_cast<char *>(buffer);
	while (size) {
		auto count = recv(3, p, size, 0);
		if (count <= 0) {
			return false;
		}
		p += count;
		size -= size_t(count);
	}
	return true;
}

int main(int argc, char **argv)
{
	if (argc == 6) {
		const std::string scenario = argv[1];
		if (scenario == "missing") {
			return 1;
		}
		if (scenario == "initializing") {
			sleep(30);
			return 0;
		}
		usleep(10000);
		const unsigned char ready = 1;
		send(3, &ready, 1, MSG_NOSIGNAL);
		std::array<unsigned char, 320 * 192 * 4> input;
		if (!Read(input.data(), input.size())) {
			return 2;
		}
		if (scenario == "crash") {
			return 3;
		}
		if (scenario == "stall") {
			sleep(30);
			return 0;
		}
		if (scenario == "delayed")
			usleep(80000);
		std::array<unsigned char, 320 * 192> mask;
		mask.fill(128);
		send(3, mask.data(), mask.size(), MSG_NOSIGNAL);
		sleep(30);
		return 0;
	}
	const auto executable = std::filesystem::canonical("/proc/self/exe").string();
	cv::Mat frame(192, 320, CV_8UC4, cv::Scalar(0)), result;
	{
		const auto start = std::chrono::steady_clock::now();
		PenguinGPUWorker worker(executable, "initializing", "", "", 3, true);
		assert(worker.Preparing());
		assert(!worker.Infer(frame, result));
		assert(worker.Preparing() && !worker.Failed());
		worker.Stop();
		assert(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(200));
	}
	for (const char *scenario : {"ready", "delayed", "missing", "crash", "stall"}) {
		PenguinGPUWorker worker(executable, scenario, "", "", 3, true, PenguinGPUWorker::BackgroundFrameBudgetMs);
		bool success = false;
		const auto start = std::chrono::steady_clock::now();
		while (worker.Preparing() && std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
			if (worker.Infer(frame, result)) {
				success = true;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
		assert(std::chrono::steady_clock::now() - start < std::chrono::seconds(2));
		if (std::string(scenario) == "ready" || std::string(scenario) == "delayed") {
			assert(success && !worker.Failed());
			assert(result.rows == 192 && result.cols == 320 && result.at<unsigned char>(0, 0) == 128);
		} else {
			assert(!success && worker.Failed());
		}
	}
	std::cout << "GPU worker: readiness, delayed response, IPC, initialization cancellation, crash and bounded timeout fallback passed\n";
}
