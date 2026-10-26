// SPDX-License-Identifier: GPL-3.0-or-later
#include "../single-task-worker.hpp"
#include <atomic>

static void require(bool condition)
{
	if (!condition) throw std::runtime_error("Worker lifetime/reuse check failed");
}
int main()
{
	std::atomic<int> calls = 0;
	std::thread::id workerId;
	const auto mainId = std::this_thread::get_id();
	{
		PenguinSingleTaskWorker worker;
		for (int i = 0; i < 100; ++i) {
			worker.submit([&] {
				if (calls == 0) workerId = std::this_thread::get_id();
				require(workerId == std::this_thread::get_id());
				require(workerId != mainId);
				++calls;
			}).get();
		}
		// A render tick must be able to inspect an unfinished job without waiting.
		// Completion publishes the frame identity and its corresponding result together.
		std::promise<void> entered, release;
		auto gate = release.get_future();
		int frame = 0, mask = 0;
		auto pending = worker.submit([&] {
			entered.set_value();
			gate.wait();
			frame = 71;
			mask = 71;
		});
		entered.get_future().wait();
		require(pending.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
		release.set_value();
		pending.get();
		require(frame == 71 && mask == frame);
		bool caught = false;
		try { worker.submit([] { throw std::runtime_error("Expected test failure"); }).get(); }
		catch (const std::runtime_error &) { caught = true; }
		require(caught);
		worker.submit([&] { ++calls; }).get();
		worker.submit([&] { ++calls; }); // Destruction must finish an accepted task.
	}
	require(calls == 102);
}
