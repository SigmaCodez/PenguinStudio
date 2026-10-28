// SPDX-License-Identifier: GPL-2.0-or-later
#include "../upload-rate.hpp"
#include <cassert>
#include <cmath>
#include <limits>
int main()
{
 UploadRate rate;
 assert(rate.Sample(500000, 1000000000ULL) == 0);
 assert(rate.Sample(1250000, 2000000000ULL) == 6000);
 assert(rate.Sample(1250000, 3000000000ULL) == 0);
 assert(rate.Sample(100, 4000000000ULL) == 0);
 assert(rate.Sample(750100, 5000000000ULL) == 6000);
 rate.Reset();
 assert(rate.Sample(9000000, 6000000000ULL) == 0);
 assert(rate.Sample(10000000, 6000000000ULL) == 0);
 assert(rate.Sample(11000000, 5000000000ULL) == 0);
 assert(std::isfinite(rate.Sample(std::numeric_limits<uint64_t>::max(), 6000000000ULL)));
 rate.Reset();
 rate.Sample(0, 1000000000ULL);
 assert(rate.Sample(1500000, 3000000000ULL) == 6000);
}
