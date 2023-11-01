#pragma once
#define _WIN32_WINNT 0x0500
#include "scope_guard.hpp"
#include <memory>
#include <stdexcept>
#include <windows.h>
#include <WinInet.h>
#include <WinNT.h>

static long PREFERENCE_DEFAULT = -1;

inline size_t stringSize(const char* string) {
	return strlen(string) + 1;
}

inline bool closeHandle(HANDLE &handle) {
	if (handle && handle != INVALID_HANDLE_VALUE) {
		if (!CloseHandle(handle)) {
			return false;
		}
	}

	handle = NULL;
	return true;
}

inline bool closeProcess(HANDLE &process) {
	if (process) {
		if (!CloseHandle(process)) {
			return false;
		}
	}

	process = NULL;
	return true;
}

inline bool closeThread(HANDLE &thread) {
	return closeProcess(thread);
}

inline bool closeInternetHandle(HINTERNET &internetHandle) {
	if (internetHandle) {
		if (!InternetCloseHandle(internetHandle)) {
			return false;
		}
	}

	internetHandle = NULL;
	return true;
}

bool showLastError(LPCSTR errorMessage);
BOOL terminateCurrentProcess();
void setCurrentThreadContext(CONTEXT &context);
bool getPreference(LPCSTR fileName, LPCSTR environmentVariableName, long &preference);