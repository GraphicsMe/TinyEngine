#pragma once

#include <vector>

static const char* LOG_TAG = "[TinyEngine]";

struct FGenericPlatformMisc
{
	static void PlatformInit() {}

	static void LocalPrint(const char* Str);

	static void LocalPrintf(const char* Format, ...);

	static void PumpMessages() {}

	static std::vector<char> ReadFile(const char* Filename);
};