#pragma once

namespace ClientConfigTimestampStability {

// The retail file manager returns the MSVCRT _wstat mtime. That CRT converts
// the UTC file time to local time with the *current* daylight-saving offset,
// then back to UTC with the offset in effect on the *file's* date. The result
// is exact only while the current season matches the file's season; otherwise
// it moves by the daylight bias at every DST transition.
//
// Unreal stores those timestamps in each generated ini's [IniVersion] section
// and regenerates the ini (discarding player keybindings and settings) when
// they no longer compare equal. Removing the current-season term yields the
// value the CRT reports during standard time, every day of the year.
inline double StableFileTimestamp(
	double crtTimestamp,
	bool daylightActive,
	long standardBiasMinutes,
	long daylightBiasMinutes) {
	// The engine reports a missing or unreadable file as -1.
	if (crtTimestamp < 0.0 || !daylightActive) return crtTimestamp;
	const double seasonalShiftSeconds =
		static_cast<double>(standardBiasMinutes - daylightBiasMinutes) * 60.0;
	return crtTimestamp - seasonalShiftSeconds;
}

}  // namespace ClientConfigTimestampStability
