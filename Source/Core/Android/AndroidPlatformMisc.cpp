#include "AndroidPlatformMisc.h"
#include <android/log.h>
#include <android_native_app_glue.h>

extern struct android_app* GNativeAndroidApp;

void FAndroidPlatformMisc::PlatformInit()
{
	FPlatformMisc::LocalPrint("Android Platform Init");
}

void FAndroidPlatformMisc::LocalPrint(const char* Str)
{
	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s", Str);
}

void FAndroidPlatformMisc::LocalPrintf(const char* Format, ...)
{
	va_list arg_list;
	va_start(arg_list, Format);
	__android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, Format, arg_list);
	va_end(arg_list);
}

void FAndroidPlatformMisc::PumpMessages()
{
	int ident;
	//int fdesc;
	int events;
	struct android_poll_source* source;
	assert(GNativeAndroidApp != nullptr);
	//timeout set to 0!!!
	while ((ident = ALooper_pollAll(0, nullptr, &events, (void**)&source)) >= 0)
	{
		// process this event
		if (source)
		{
			source->process(GNativeAndroidApp, source);
		}
	}
}

std::vector<char> FAndroidPlatformMisc::ReadFile(const char* Filename)
{
    assert(GNativeAndroidApp != nullptr);
	std::vector<char> Buffer;
    AAsset *file = AAssetManager_open(GNativeAndroidApp->activity->assetManager, Filename, AASSET_MODE_BUFFER);
    if (!file)
	{
    	FAndroidPlatformMisc::LocalPrintf("Load file failed: %s", Filename);
    	return Buffer;
	}
    size_t FileLength = AAsset_getLength(file);
	FAndroidPlatformMisc::LocalPrintf("Loaded file:%s size:%zu", Filename, FileLength);

    if (FileLength > 0) {
        Buffer.resize(FileLength);
        AAsset_read(file, &Buffer[0], FileLength);
    }
    return Buffer;
}