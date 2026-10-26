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
		bool caught = false;
		try { worker.submit([] { throw std::runtime_error("Expected test failure"); }).get(); }
		catch (const std::runtime_error &) { caught = true; }
		require(caught);
		worker.submit([&] { ++calls; }).get();
		worker.submit([&] { ++calls; }); // Destruction must finish an accepted task.
	}
	require(calls == 102);
}
