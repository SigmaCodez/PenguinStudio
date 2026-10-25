// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <chrono>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <filesystem>
#include <poll.h>
#include <spawn.h>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <opencv2/imgproc.hpp>

extern char **environ;

// Driver initialization and compilation run in a child process. Neither a slow
// compiler nor a GPU driver crash can block Qt or terminate Penguin Studio.
class PenguinGPUWorker {
	int socket = -1;
	pid_t process = -1;
	bool ready = false;
	int budgetMs = 40;
	const char *failure = "worker initialization failed";
	std::chrono::steady_clock::time_point started;

	bool Transfer(void *buffer, size_t size, bool write, std::chrono::steady_clock::time_point deadline)
	{
		auto *bytes = static_cast<unsigned char *>(buffer);
		while (size) {
			const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
						       deadline - std::chrono::steady_clock::now())
						       .count();
			if (remaining <= 0) {
				failure = "frame response timed out";
				return false;
			}
			pollfd poller{socket, short(write ? POLLOUT : POLLIN), 0};
			const int result = poll(&poller, 1, int(remaining));
			if (result < 0 && errno == EINTR) {
				continue;
			}
			if (result <= 0 || (poller.revents & (POLLERR | POLLHUP | POLLNVAL))) {
				failure = result == 0 ? "frame response timed out" : "worker connection closed";
				return false;
			}
			const auto count = write ? send(socket, bytes, size, MSG_NOSIGNAL)
						 : recv(socket, bytes, size, 0);
			if (count < 0 && (errno == EINTR || errno == EAGAIN)) {
				continue;
			}
			if (count <= 0) {
				failure = "worker connection closed";
				return false;
			}
			bytes += count;
			size -= size_t(count);
		}
		return true;
	}

public:
	PenguinGPUWorker(const std::string &executable, const std::string &runtime, const std::string &models,
			 const std::string &cache, int threads, bool hands, int transferBudgetMs = 40)
		: budgetMs(transferBudgetMs)
	{
		int pair[2];
		if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, pair)) {
			return;
		}
		// Reserve descriptors above the child's fixed IPC descriptor before dup2.
		socket = fcntl(pair[0], F_DUPFD_CLOEXEC, 10);
		const int child = fcntl(pair[1], F_DUPFD_CLOEXEC, 10);
		close(pair[0]);
		close(pair[1]);
		if (socket < 0 || child < 0) {
			if (child >= 0) {
				close(child);
			}
			Stop();
			return;
		}
		posix_spawn_file_actions_t actions;
		posix_spawn_file_actions_init(&actions);
		posix_spawn_file_actions_adddup2(&actions, child, 3);
		posix_spawn_file_actions_addclose(&actions, child);
		posix_spawn_file_actions_addclose(&actions, socket);
		std::vector<std::string> values{executable,       runtime, models, cache, std::to_string(threads),
						hands ? "1" : "0"};
		std::vector<char *> arguments;
		for (auto &value : values) {
			arguments.push_back(value.data());
		}
		arguments.push_back(nullptr);
		const auto gpuDirectory = std::filesystem::path(runtime).parent_path();
		const auto rocm = gpuDirectory / "rocm";
		std::vector<std::string> overrides;
		std::string libraries = "LD_LIBRARY_PATH=" + gpuDirectory.string() + ":" + (rocm / "lib").string() +
					":" + (rocm / "lib/migraphx/lib").string();
		if (const char *existing = getenv("LD_LIBRARY_PATH")) {
			libraries += ":" + std::string(existing);
		}
		overrides.push_back(libraries);
		if (std::filesystem::is_directory(rocm)) {
			overrides.push_back("HIP_PATH=" + rocm.string());
			overrides.push_back("ROCM_PATH=" + rocm.string());
		}
		std::vector<char *> environment;
		for (char **entry = environ; *entry; ++entry) {
			if (std::string(*entry).rfind("LD_PRELOAD=", 0) != 0 &&
			    std::string(*entry).rfind("LD_LIBRARY_PATH=", 0) != 0 &&
			    (!std::filesystem::is_directory(rocm) ||
			     (std::string(*entry).rfind("HIP_PATH=", 0) != 0 &&
			      std::string(*entry).rfind("ROCM_PATH=", 0) != 0))) {
				environment.push_back(*entry);
			}
		}
		for (auto &value : overrides) {
			environment.push_back(value.data());
		}
		environment.push_back(nullptr);
		const int error = posix_spawn(&process, executable.c_str(), &actions, nullptr, arguments.data(),
					      environment.data());
		posix_spawn_file_actions_destroy(&actions);
		close(child);
		if (error) {
			process = -1;
			Stop();
			return;
		}
		fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
		started = std::chrono::steady_clock::now();
	}
	~PenguinGPUWorker() { Stop(); }
	PenguinGPUWorker(const PenguinGPUWorker &) = delete;
	PenguinGPUWorker &operator=(const PenguinGPUWorker &) = delete;
	void Stop()
	{
		if (socket >= 0) {
			close(socket);
		}
		socket = -1;
		ready = false;
		if (process > 0) {
			kill(process, SIGKILL);
			while (waitpid(process, nullptr, 0) < 0 && errno == EINTR) {
			}
		}
		process = -1;
	}
	const char *Failure() const { return failure; }
	bool Failed() const { return socket < 0; }
	bool Preparing() const { return socket >= 0 && !ready; }
	bool Infer(const cv::Mat &frame, cv::Mat &mask)
	{
		if (socket < 0) {
			return false;
		}
		if (!ready) {
			pollfd poller{socket, POLLIN, 0};
			const int status = poll(&poller, 1, 0);
			if (status == 0 && std::chrono::steady_clock::now() - started < std::chrono::minutes(3)) {
				return false;
			}
			unsigned char result = 0;
			if (status <= 0 ||
			    !Transfer(&result, 1, false,
				      std::chrono::steady_clock::now() + std::chrono::milliseconds(budgetMs)) ||
			    result != 1) {
				Stop();
				return false;
			}
			ready = true;
		}
		cv::Mat input;
		cv::resize(frame, input, cv::Size(320, 192));
		mask.create(192, 320, CV_8UC1);
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(budgetMs);
		if (!Transfer(input.data, 320 * 192 * 4, true, deadline) ||
		    !Transfer(mask.data, 320 * 192, false, deadline)) {
			mask.release();
			Stop();
			return false;
		}
		return true;
	}
};
