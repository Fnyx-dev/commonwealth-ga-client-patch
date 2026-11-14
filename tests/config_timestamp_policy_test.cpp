#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampPolicy.hpp"

#include <cassert>
#include <cstdio>
#include <limits>

using ClientConfigTimestampStability::IniTimestampsMatch;

int main() {
	// Unchanged files match exactly, as in retail.
	assert(IniTimestampsMatch(1772224066.0, 1772224066.0));

	// A summer file read in winter by the retail CRT (stored 1792519707,
	// true UTC 1792523307) and the reverse both stay valid, so DST changes
	// and switching to Wine's exact CRT never reset settings.
	assert(IniTimestampsMatch(1792519707.0, 1792523307.0));
	assert(IniTimestampsMatch(1792523307.0, 1792519707.0));

	// Any other difference is a real edit and still regenerates the ini.
	assert(!IniTimestampsMatch(1792523307.0, 1792523308.0));
	assert(!IniTimestampsMatch(1792523307.0, 1792523307.0 + 1800.0));
	assert(!IniTimestampsMatch(1792523307.0, 1792523307.0 + 3599.0));
	assert(!IniTimestampsMatch(1792523307.0, 1792523307.0 + 3601.0));
	assert(!IniTimestampsMatch(1792523307.0, 1792523307.0 + 7200.0));

	// A missing file (-1) only matches another -1.
	assert(IniTimestampsMatch(-1.0, -1.0));
	assert(!IniTimestampsMatch(-1.0, 3599.0));
	assert(!IniTimestampsMatch(3599.0, -1.0));
	assert(!IniTimestampsMatch(1792523307.0, -1.0));

	// Unordered values never match, like the retail parity check.
	const double nan = std::numeric_limits<double>::quiet_NaN();
	assert(!IniTimestampsMatch(nan, nan));
	assert(!IniTimestampsMatch(nan, 1792523307.0));

	std::puts("config timestamp policy tests passed");
	return 0;
}
