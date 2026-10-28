// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>

// One reusable companion thread, with an explicit completion future per frame.
// The caller consumes each result before submitting another and owns frame admission.
class PenguinSingleTaskWorker {
	std::mutex mutex;
	std::condition_variable wake;
	std::packaged_task<void()> pending;
	bool stopping = false;
	std::thread thread;

public:
	PenguinSingleTaskWorker() : thread([this] {
		for (;;) {
			std::packaged_task<void()> task;
			{
				std::unique_lock lock(mutex);
				wake.wait(lock, [this] { return stopping || pending.valid(); });
				if (!pending.valid()) return;
				task = std::move(pending);
			}
			task(); // packaged_task transfers exceptions to the calling frame.
		}
	}) {}
	PenguinSingleTaskWorker(const PenguinSingleTaskWorker &) = delete;
	PenguinSingleTaskWorker &operator=(const PenguinSingleTaskWorker &) = delete;
	~PenguinSingleTaskWorker()
	{
		{
			std::lock_guard lock(mutex);
			stopping = true;
		}
		wake.notify_one();
		thread.join(); // Drain any accepted task before destroying captured state.
	}
	std::future<void> submit(std::function<void()> function)
	{
		std::packaged_task<void()> task(std::move(function));
		auto result = task.get_future();
		{
			std::lock_guard lock(mutex);
			if (stopping || pending.valid()) throw std::logic_error("Companion task already pending");
			pending = std::move(task);
		}
		wake.notify_one();
		return result;
	}
};
