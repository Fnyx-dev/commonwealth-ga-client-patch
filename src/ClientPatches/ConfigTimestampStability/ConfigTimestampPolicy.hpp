#pragma once

namespace ClientConfigTimestampStability {

// The retail file manager returns the MSVCRT _wstat mtime. That CRT converts
// the UTC file time to local time with the *current* daylight-saving offset,
// then back to UTC with the offset in effect on the *file's* date. The result
// is exact only while the current season matches the file's season; otherwise
// it is off by exactly the daylight bias (one hour). Wine's CRT is exact.
//
// Unreal stores those timestamps in each generated ini's [IniVersion] section
// and regenerates the ini (discarding player keybindings and settings) when
// they no longer compare equal. Accepting a difference of exactly one hour
// keeps every value already stored by any season or CRT valid, so neither DST
// transitions nor installing this fix trigger a reset. Real edits to a source
// ini still change its timestamp by some other amount and regenerate as before.

constexpr double kDaylightBiasSeconds = 3600.0;

// Retail compares with an exact equality test (fucomip). Timestamps are whole
// seconds, and -1 marks a missing file; -1 still only matches -1.
inline bool IniTimestampsMatch(double stored, double current) {
	if (stored == current) return true;
	if (stored < 0.0 || current < 0.0) return false;
	const double difference = stored - current;
	return difference == kDaylightBiasSeconds || difference == -kDaylightBiasSeconds;
}

}  // namespace ClientConfigTimestampStability
