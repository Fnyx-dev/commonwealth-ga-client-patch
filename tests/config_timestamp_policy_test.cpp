#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampPolicy.hpp"

#include <cassert>
#include <cstdio>

using ClientConfigTimestampStability::StableFileTimestamp;

int main() {
	// Central European time: StandardBias 0, DaylightBias -60 minutes.
	// A file written in winter (UTC 1772224066) and one written in summer
	// (UTC 1792523307), as the retail CRT reports them in each season.
	const double winterFileUtc = 1772224066.0;
	const double summerFileUtc = 1792523307.0;
	const double winterFileDuringWinter = winterFileUtc;
	const double summerFileDuringWinter = summerFileUtc - 3600.0;
	const double winterFileDuringSummer = winterFileUtc + 3600.0;
	const double summerFileDuringSummer = summerFileUtc;

	// Standard time passes the CRT value through unchanged.
	assert(StableFileTimestamp(winterFileDuringWinter, false, 0, -60) ==
		winterFileDuringWinter);
	assert(StableFileTimestamp(summerFileDuringWinter, false, 0, -60) ==
		summerFileDuringWinter);

	// Daylight time maps to the same values, so [IniVersion] stays equal.
	assert(StableFileTimestamp(winterFileDuringSummer, true, 0, -60) ==
		winterFileDuringWinter);
	assert(StableFileTimestamp(summerFileDuringSummer, true, 0, -60) ==
		summerFileDuringWinter);

	// Zones with a non-hour daylight bias (Lord Howe Island: 30 minutes).
	assert(StableFileTimestamp(1000.0 + 1800.0, true, 0, -30) == 1000.0);

	// Zones without daylight saving are never adjusted.
	assert(StableFileTimestamp(winterFileUtc, false, 0, 0) == winterFileUtc);

	// The engine's failure value is preserved in every season.
	assert(StableFileTimestamp(-1.0, true, 0, -60) == -1.0);
	assert(StableFileTimestamp(-1.0, false, 0, -60) == -1.0);

	std::puts("config timestamp policy tests passed");
	return 0;
}
