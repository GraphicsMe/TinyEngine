#pragma once

#include "GenericPlatform/GenericPlatformMisc.h"

struct FWindowsPlatformMisc : public FGenericPlatformMisc
{
	static void PlatformInit();

	static void PumpMessages();
};

typedef FWindowsPlatformMisc FPlatformMisc;