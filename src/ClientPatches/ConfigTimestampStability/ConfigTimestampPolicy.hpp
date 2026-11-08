#pragma once

#include <cstdint>

namespace ClientConfigTimestampStability {

// The retail file manager returns the MSVCRT _wstat mtime. That CRT converts
// the UTC file time to local time with the *current* daylight-saving offset,
// then back to UTC with the offset in effect on the *file's* date. The result
// is exact only while the current season matches the file's season; otherwise
// it moves by the daylight bias at every DST transition.
//
// Unreal stores those timestamps in each generated ini's [IniVersion] section
// and regenerates the ini (discarding player keybindings and settings) when
// they no longer compare equal. Reporting the plain UTC write time keeps the
// value identical in every season and with every CRT, including Wine's.

constexpr std::uint64_t kFileTimeTicksPerSecond = 10000000ULL;
// Seconds between the FILETIME epoch (1601-01-01) and the Unix epoch.
constexpr std::uint64_t kUnixEpochFileTimeSeconds = 11644473600ULL;

// Converts a FILETIME (100 ns ticks since 1601, UTC) to whole Unix seconds,
// the same resolution as the st_mtime the engine received from the CRT.
inline double UtcTimestampFromFileTime(std::uint64_t fileTime) {
	const std::int64_t seconds =
		static_cast<std::int64_t>(fileTime / kFileTimeTicksPerSecond) -
		static_cast<std::int64_t>(kUnixEpochFileTimeSeconds);
	return static_cast<double>(seconds);
}

}  // namespace ClientConfigTimestampStability
