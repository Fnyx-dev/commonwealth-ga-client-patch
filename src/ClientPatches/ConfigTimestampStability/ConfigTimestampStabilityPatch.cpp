#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampStabilityPatch.hpp"

#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampPolicy.hpp"
#include "src/Utils/HookBase.hpp"

namespace {

class GetFileTimestampHook : public HookBase<
	double(__fastcall*)(void*, void*, const wchar_t*),
	// FFileManagerWindows::GetFileTimestamp (vtable slot 15): returns the
	// _wstat64i32 st_mtime as a double, or -1 when the file cannot be read.
	// The config cache compares these values with [IniVersion] entries.
	0x1130F7A0,
	GetFileTimestampHook> {
public:
	static double __fastcall Call(
		void* fileManager,
		void* edx,
		const wchar_t* filename) {
		const double timestamp = m_original(fileManager, edx, filename);
		TIME_ZONE_INFORMATION zone{};
		const DWORD zoneId = GetTimeZoneInformation(&zone);
		return ClientConfigTimestampStability::StableFileTimestamp(
			timestamp,
			zoneId == TIME_ZONE_ID_DAYLIGHT,
			zone.StandardBias,
			zone.DaylightBias);
	}
};

}  // namespace

LONG ClientConfigTimestampStabilityPatch::Install() {
	return GetFileTimestampHook::Install();
}
