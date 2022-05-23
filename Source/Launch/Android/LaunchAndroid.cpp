#include <android_native_app_glue.h>
#define VK_USE_PLATFORM_ANDROID_KHR 1
#include "vulkan_wrapper.h"

#include "HAL/PlatformMisc.h"

extern int GuardedMain();

struct android_app* GNativeAndroidApp = nullptr;

static bool GWindowInitialized = false;
extern bool GIsRequestingExit;

void Android_handle_cmd(android_app* app, int32_t cmd)
{
    switch (cmd)
    {
        case APP_CMD_INIT_WINDOW:
            FPlatformMisc::LocalPrint("Android Window Initialized!");
            GWindowInitialized = true;
            GuardedMain();
            break;
        case APP_CMD_TERM_WINDOW:
            FPlatformMisc::LocalPrint("Android Window Terminated!");
            GIsRequestingExit = true;
            break;
        default:
            FPlatformMisc::LocalPrintf("Android Event not handed: %d", cmd);
            break;
    }
}

void android_main(struct android_app* app)
{
    FPlatformMisc::LocalPrint("This is Android platform!");

    GNativeAndroidApp = app;
    app->onAppCmd = Android_handle_cmd;

    if (!InitVulkan())
    {
        FPlatformMisc::LocalPrint("Vulkan not supported, Exit!");
        return;
    }
    else
    {
        FPlatformMisc::LocalPrint("InitVulkan Success!");
    }
    while (!GWindowInitialized)
    {
        FPlatformMisc::PumpMessages();
    }
}
