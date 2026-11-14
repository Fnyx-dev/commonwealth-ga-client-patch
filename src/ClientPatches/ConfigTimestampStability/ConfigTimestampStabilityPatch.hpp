#pragma once

#include "src/pch.hpp"

// Treats [IniVersion] timestamps that differ by exactly one hour as unchanged
// so generated config files are not reset at DST transitions.
//
// The engine checks its inis within moments of process start, usually before
// the full executable hash finishes, so this hook cannot join the main startup
// transaction. InstallEarly() runs first on the installation worker, verifies
// the loaded image header and every byte it patches or jumps to, and attaches
// in its own transaction. FinishInstall() logs the result once logging is up
// and removes the hook if the full guard rejects the executable.
class ClientConfigTimestampStabilityPatch {
public:
	static void InstallEarly();
	static void FinishInstall(bool executableSupported);
	static bool IsInstalled();
};
