#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampPolicy.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>

using ClientConfigTimestampStability::kFileTimeTicksPerSecond;
using ClientConfigTimestampStability::kUnixEpochFileTimeSeconds;
using ClientConfigTimestampStability::UtcTimestampFromFileTime;

namespace {

std::uint64_t FileTimeFromUnixSeconds(std::uint64_t seconds, std::uint64_t extraTicks = 0) {
	return (seconds + kUnixEpochFileTimeSeconds) * kFileTimeTicksPerSecond + extraTicks;
}

}  // namespace

int main() {
	// The Unix epoch itself.
	assert(UtcTimestampFromFileTime(FileTimeFromUnixSeconds(0)) == 0.0);

	// Files written in winter (Feb 27) and summer (Oct 20) of a CET install.
	// The retail CRT reported the summer file 3600 s early during winter and
	// the winter file 3600 s late during summer; UTC needs no season input.
	assert(UtcTimestampFromFileTime(FileTimeFromUnixSeconds(1772224066)) == 1772224066.0);
	assert(UtcTimestampFromFileTime(FileTimeFromUnixSeconds(1792523307)) == 1792523307.0);

	// Sub-second ticks truncate to whole seconds, like st_mtime.
	assert(UtcTimestampFromFileTime(
		FileTimeFromUnixSeconds(1792523307, kFileTimeTicksPerSecond - 1)) == 1792523307.0);

	// Times past 2038 stay exact.
	assert(UtcTimestampFromFileTime(FileTimeFromUnixSeconds(4102444800ULL)) == 4102444800.0);

	std::puts("config timestamp policy tests passed");
	return 0;
}
