#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampStabilityPatch.hpp"

#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampPolicy.hpp"
#include "src/Utils/ClientExecutableGuard/ClientExecutableGuard.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>

// appCheckIniForOutdatedness (0x112BB260) loops over the [IniVersion] keys of
// the generated ini and of the freshly read source inis. At 0x112BB388 both
// values are doubles on the stack ([esp+0x5C] and [esp+0x4C]) and the retail
// code runs `fld; fld; fucomip; fstp; lahf; test; jp`, branching to
// 0x112BB39A when they are equal (next key) or 0x112BB3A8 when they differ
// (mark the ini outdated). The stub replaces that comparison and jumps to the
// same two targets. The x87 stack is untouched, and eax is dead at both
// targets (retail clobbers ah with lahf).
extern "C" {
void* g_iniVersionCompare = reinterpret_cast<void*>(0x112BB388u);
void* g_iniVersionMatch = reinterpret_cast<void*>(0x112BB39Au);
void* g_iniVersionOutdated = reinterpret_cast<void*>(0x112BB3A8u);
void IniVersionCompareStub();
__attribute__((force_align_arg_pointer)) int IniVersionValuesMatch(
	const double* current,
	const double* stored);
}

// 0x4C/0x5C + 0x20 (pushal) + 0x4 (pushfl) = 0x70/0x80; the second lea runs
// after one push, so 0x70 + 0x4 = 0x74.
asm(
	".text\n"
	".globl _IniVersionCompareStub\n"
	"_IniVersionCompareStub:\n"
	"	pushal\n"
	"	pushfl\n"
	"	leal 0x80(%esp), %eax\n"
	"	pushl %eax\n"
	"	leal 0x74(%esp), %eax\n"
	"	pushl %eax\n"
	"	call _IniVersionValuesMatch\n"
	"	addl $8, %esp\n"
	"	testl %eax, %eax\n"
	"	jz 1f\n"
	"	popfl\n"
	"	popal\n"
	"	jmp *_g_iniVersionMatch\n"
	"1:\n"
	"	popfl\n"
	"	popal\n"
	"	jmp *_g_iniVersionOutdated\n");

// Runs on the game thread, possibly before logging is initialized: no logging.
int IniVersionValuesMatch(const double* current, const double* stored) {
	return ClientConfigTimestampStability::IniTimestampsMatch(*stored, *current) ? 1 : 0;
}

namespace {

// Retail bytes from the compare at 0x112BB388 through `mov ebx, 1` at
// 0x112BB3A8: both relocated fld instructions plus both jump targets.
constexpr unsigned char kCompareSiteBytes[] = {
	0xDD, 0x44, 0x24, 0x4C,              // fld qword [esp+0x4C]
	0xDD, 0x44, 0x24, 0x5C,              // fld qword [esp+0x5C]
	0xDF, 0xE9,                          // fucomip st(1)
	0xDD, 0xD8,                          // fstp st(0)
	0x9F,                                // lahf
	0xF6, 0xC4, 0x44,                    // test ah, 0x44
	0x7A, 0x0E,                          // jp 0x112BB3A8
	0x83, 0xC6, 0x01,                    // 0x112BB39A: add esi, 1
	0x83, 0xFF, 0x01,                    // cmp edi, 1
	0x0F, 0x84, 0x69, 0xFF, 0xFF, 0xFF,  // je 0x112BB30F
	0xEB, 0x05,                          // jmp 0x112BB3AD
	0xBB, 0x01, 0x00, 0x00, 0x00,        // 0x112BB3A8: mov ebx, 1
};

enum class EarlyResult { NotAttempted, Installed, HeaderMismatch, BytesMismatch, DetoursFailed };

EarlyResult s_earlyResult = EarlyResult::NotAttempted;
LONG s_earlyError = NO_ERROR;

LONG RunSingleHookTransaction(bool attach) {
	LONG result = ::DetourTransactionBegin();
	if (result != NO_ERROR) return result;
	result = ::DetourUpdateThread(::GetCurrentThread());
	if (result == NO_ERROR) {
		result = attach
			? ::DetourAttach(&g_iniVersionCompare, reinterpret_cast<PVOID>(&IniVersionCompareStub))
			: ::DetourDetach(&g_iniVersionCompare, reinterpret_cast<PVOID>(&IniVersionCompareStub));
	}
	if (result != NO_ERROR) {
		::DetourTransactionAbort();
		return result;
	}
	return ::DetourTransactionCommit();
}

}  // namespace

void ClientConfigTimestampStabilityPatch::InstallEarly() {
	if (!ClientExecutableGuard::LoadedImageHeaderMatches()) {
		s_earlyResult = EarlyResult::HeaderMismatch;
		return;
	}
	if (std::memcmp(
			reinterpret_cast<const void*>(0x112BB388u),
			kCompareSiteBytes,
			sizeof(kCompareSiteBytes)) != 0) {
		s_earlyResult = EarlyResult::BytesMismatch;
		return;
	}
	// Detours relocates the two 4-byte fld instructions into a trampoline that
	// is never used: the stub always leaves through one of the retail targets.
	s_earlyError = RunSingleHookTransaction(true);
	s_earlyResult = s_earlyError == NO_ERROR ? EarlyResult::Installed : EarlyResult::DetoursFailed;
}

void ClientConfigTimestampStabilityPatch::FinishInstall(bool executableSupported) {
	if (s_earlyResult == EarlyResult::Installed && !executableSupported) {
		const LONG result = RunSingleHookTransaction(false);
		Logger::Log(
			"clientpatch",
			"[config] IniVersion hook removed for unsupported executable (%ld)\n",
			static_cast<long>(result));
		s_earlyResult = EarlyResult::NotAttempted;
		return;
	}
	switch (s_earlyResult) {
		case EarlyResult::Installed:
			Logger::Log("clientpatch", "[config] IniVersion hook installed before startup checks\n");
			break;
		case EarlyResult::HeaderMismatch:
			if (executableSupported) {
				Logger::Log("clientpatch", "[config] IniVersion hook skipped: image header mismatch\n");
			}
			break;
		case EarlyResult::BytesMismatch:
			Logger::Log("clientpatch", "[config] IniVersion hook skipped: unexpected code bytes\n");
			break;
		case EarlyResult::DetoursFailed:
			Logger::Log(
				"clientpatch",
				"[config] IniVersion hook failed (%ld)\n",
				static_cast<long>(s_earlyError));
			break;
		case EarlyResult::NotAttempted:
			break;
	}
}

bool ClientConfigTimestampStabilityPatch::IsInstalled() {
	return s_earlyResult == EarlyResult::Installed;
}
