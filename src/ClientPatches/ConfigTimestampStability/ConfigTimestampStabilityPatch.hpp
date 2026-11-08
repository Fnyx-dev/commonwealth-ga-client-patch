#pragma once

#include "src/pch.hpp"

// Keeps engine file timestamps independent of the current daylight-saving
// season so generated config files are not reset at every DST transition.
class ClientConfigTimestampStabilityPatch {
public:
	static LONG Install();
};
