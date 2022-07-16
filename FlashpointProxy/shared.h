#pragma once
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <WinNT.h>

inline size_t stringSize(const char* string) {
	return strlen(string) + 1;
}

bool showLastError(LPCSTR errorMessage);
BOOL terminateCurrentProcess();
void setCurrentThreadContext(CONTEXT &context);