#pragma once

#include <GenericPlatform/GenericPlatformMisc.h>

struct FAndroidPlatformMisc : public FGenericPlatformMisc
{
	static void PlatformInit();
	static void LocalPrint(const char* Str);
	static void LocalPrintf(const char* Format, ...);
	static void PumpMessages();
	static std::vector<char> ReadFile(const char* Filename);
};

typedef FAndroidPlatformMisc FPlatformMisc;