#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampStabilityPatch.hpp"

#include "src/ClientPatches/ConfigTimestampStability/ConfigTimestampPolicy.hpp"

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

int IniVersionValuesMatch(const double* current, const double* stored) {
	return ClientConfigTimestampStability::IniTimestampsMatch(*stored, *current) ? 1 : 0;
}

LONG ClientConfigTimestampStabilityPatch::Install() {
	// Detours relocates the two 4-byte fld instructions into a trampoline that
	// is never used: the stub always leaves through one of the retail targets.
	return ::DetourAttach(&g_iniVersionCompare, reinterpret_cast<PVOID>(&IniVersionCompareStub));
}
