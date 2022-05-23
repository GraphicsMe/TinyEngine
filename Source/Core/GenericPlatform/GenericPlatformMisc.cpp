#include "GenericPlatformMisc.h"
#include <stdio.h>
#include <stdarg.h>
#include <fstream>
#include <sstream>

void FGenericPlatformMisc::LocalPrint(const char* Str)
{
	printf("%s: %s\n", LOG_TAG, Str);
}

void FGenericPlatformMisc::LocalPrintf(const char* Format, ...)
{
	va_list arg_list;
	va_start(arg_list, Format);
	fprintf(stdout, "%s: ", LOG_TAG);
	vfprintf(stdout, Format, arg_list);
	va_end(arg_list);
}

std::vector<char> FGenericPlatformMisc::ReadFile(const char* Filename)
{
	std::stringstream ss;
	ss << "../../Resource/" << Filename;
	std::ifstream File(ss.str(), std::ios::ate | std::ios::binary);
	if (!File.is_open())
	{
		FGenericPlatformMisc::LocalPrintf("Failed to read file: %s", Filename);
		throw std::runtime_error("Failed to open file");
	}
	size_t FileSize = (size_t)File.tellg();
	std::vector<char> Buffer(FileSize);
	File.seekg(0);
	File.read(Buffer.data(), FileSize);
	File.close();
	return Buffer;
}

