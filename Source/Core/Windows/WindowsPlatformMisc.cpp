#include "WindowsPlatformMisc.h"
#include <Windows.h>


void FWindowsPlatformMisc::PlatformInit()
{
	FPlatformMisc::LocalPrint("Windows Platform Init");
}

void FWindowsPlatformMisc::PumpMessages()
{
	MSG msg = {};
	while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}