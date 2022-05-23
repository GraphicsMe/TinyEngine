#include "HAL/PlatformMisc.h"


extern int GuardedMain();

//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
int main()
{
	FPlatformMisc::LocalPrint("This is Windows platform");

	GuardedMain();

	return 0;
}