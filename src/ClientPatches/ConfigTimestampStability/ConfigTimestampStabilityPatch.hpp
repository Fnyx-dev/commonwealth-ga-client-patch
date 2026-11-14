#pragma once

#include "src/pch.hpp"

// Treats [IniVersion] timestamps that differ by exactly one hour as unchanged
// so generated config files are not reset at DST transitions.
class ClientConfigTimestampStabilityPatch {
public:
	static LONG Install();
};
