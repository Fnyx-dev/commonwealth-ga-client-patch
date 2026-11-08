#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampStabilityPatch.hpp"

#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampPolicy.hpp"
#include "src/Utils/HookBase.hpp"

#include <cstdint>

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
		// Keep the engine's -1 for missing or unreadable files.
		if (timestamp < 0.0) return timestamp;

		WIN32_FILE_ATTRIBUTE_DATA attributes{};
		if (!GetFileAttributesExW(filename, GetFileExInfoStandard, &attributes)) {
			return timestamp;
		}
		const std::uint64_t writeTime =
			(static_cast<std::uint64_t>(attributes.ftLastWriteTime.dwHighDateTime) << 32) |
			attributes.ftLastWriteTime.dwLowDateTime;
		return ClientConfigTimestampStability::UtcTimestampFromFileTime(writeTime);
	}
};

}  // namespace

LONG ClientConfigTimestampStabilityPatch::Install() {
	return GetFileTimestampHook::Install();
}
