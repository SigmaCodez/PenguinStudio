// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>

// A byte counter is cumulative only within one connection. Reset baselines on
// reconnect/counter rollback; use monotonic elapsed time, never timer tick counts.
class UploadRate {
	uint64_t bytes = 0, timestamp = 0;
	bool initialized = false;
public:
	void Reset() { bytes = timestamp = 0; initialized = false; }
	double Sample(uint64_t current, uint64_t now)
	{
		const bool valid = initialized && current >= bytes && now > timestamp;
		const double rate = valid ? double(current - bytes) * 8000000.0 / double(now - timestamp) : 0.0;
		bytes = current;
		timestamp = now;
		initialized = true;
		return rate; // decimal kilobits per second
	}
};
