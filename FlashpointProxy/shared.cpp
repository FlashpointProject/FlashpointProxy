#include "shared.h"
#include <windows.h>
#include <winnt.h>
#include <string>
#include <sstream>

static LPCSTR MESSAGE_BOX_CAPTION = "Flashpoint Proxy";

bool showLastError(LPCSTR errorMessage) {
	if (!errorMessage) {
		return false;
	}

	std::ostringstream oStringStream;
	oStringStream << errorMessage << " (" << GetLastError() << ")";

	if (!MessageBox(NULL, oStringStream.str().c_str(), MESSAGE_BOX_CAPTION, MB_OK | MB_ICONERROR)) {
		return false;
	}
	return true;
}

BOOL terminateCurrentProcess() {
	return TerminateProcess(GetCurrentProcess(), GetLastError());
}

// this is defined in safeseh.asm
// it adds the exception handler to the SafeSEH image
// which prevents SEH exploits
extern "C" int __stdcall _except_handler_safeseh();

static const size_t CONTEXT_SIZE = sizeof(CONTEXT);
static const DWORD SET_CURRENT_THREAD_CONTEXT_EXCEPTION_CODE = 0xE0000000 | (CONTEXT_SIZE & 0x0FFFFFFF);

static const DWORD SET_CURRENT_THREAD_CONTEXT_EXCEPTION_FLAGS = 0;
static const DWORD SET_CURRENT_THREAD_CONTEXT_NUMBER_OF_ARGUMENTS = 1;

#if !defined _AMD64_ || !defined _IA64_
extern "C" EXCEPTION_DISPOSITION __cdecl _except_handler(struct _EXCEPTION_RECORD* ExceptionRecord, void* EstablisherFrame, struct _CONTEXT* ContextRecord, void* DispatcherContext) {
	// if there is an error with the arguments
	// let the next "real" exception handler handle the error
	if (!ExceptionRecord) {
		return ExceptionContinueSearch;
	}

	if (ExceptionRecord->ExceptionCode != SET_CURRENT_THREAD_CONTEXT_EXCEPTION_CODE ||
		ExceptionRecord->ExceptionFlags != SET_CURRENT_THREAD_CONTEXT_EXCEPTION_FLAGS ||
		ExceptionRecord->NumberParameters < SET_CURRENT_THREAD_CONTEXT_NUMBER_OF_ARGUMENTS) {
		return ExceptionContinueSearch;
	}

	if (!ExceptionRecord->ExceptionInformation[0]) {
		return ExceptionContinueSearch;
	}

	if (!ContextRecord) {
		return ExceptionContinueSearch;
	}

	if (!EstablisherFrame) {
		return ExceptionContinueSearch;
	}

	DWORD record = *(PDWORD)EstablisherFrame;

	// set the new thread context to the one passed in as an argument
	*ContextRecord = *(PCONTEXT)ExceptionRecord->ExceptionInformation[0];

	__asm {
		// remove our EXCEPTION_REGISTRATION record
		mov eax, record // address of previous record
		mov ecx, fs:[0] // address of current record
		mov [ecx], eax // set pointer to previous record at address of current record
	}
	return ExceptionContinueExecution;
}
#endif

void setCurrentThreadContext(CONTEXT &context) {
	#if defined _AMD64_ || defined _IA64_
	// intended Windows function for this purpose, 64-bit only :(
	RtlRestoreContext(&context, NULL);
	#else
	ULONG_PTR setCurrentThreadContextArguments[SET_CURRENT_THREAD_CONTEXT_NUMBER_OF_ARGUMENTS] = { (ULONG_PTR)&context };

	// pass the new thread context as an argument
	__asm {
		// build EXCEPTION_REGISTRATION record
		push _except_handler_safeseh // address of handler function
		push fs:[0] // address of previous record
		mov fs:[0], esp // install new EXCEPTION_REGISTRATION
	}

	// raise the Current Thread Context exception
	RaiseException(SET_CURRENT_THREAD_CONTEXT_EXCEPTION_CODE, SET_CURRENT_THREAD_CONTEXT_EXCEPTION_FLAGS, SET_CURRENT_THREAD_CONTEXT_NUMBER_OF_ARGUMENTS, setCurrentThreadContextArguments);

	__asm {
		// remove our EXCEPTION_REGISTRATION record
		mov eax, [esp] // get pointer to previous record
		mov fs:[0], eax // install previous record
		add esp, 00000008h // clean our EXCEPTION_REGISTRATION off stack
	}
	#endif
}

bool preferenceStringToLong(LPCSTR preferenceString, long &preference) {
	if (!preferenceString) {
		return false;
	}

	char* endPointer = 0;

	errno = 0;
	preference = strtol(preferenceString, &endPointer, 0);

	if (preferenceString == endPointer
		|| (!preference && errno != 0)) {
		preference = PREFERENCE_DEFAULT;
	}
	return true;
}

bool getFilePreference(LPCSTR fileName, long &preference) {
	preference = PREFERENCE_DEFAULT;

	if (!fileName) {
		return false;
	}

	bool result = true;

	HANDLE file = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	SCOPE_EXIT {
		if (!closeHandle(file)) {
			result = false;
		}
	};

	if (file && file != INVALID_HANDLE_VALUE) {
		const DWORD PREFERENCE_STRING_SIZE = 16;
		CHAR preferenceString[PREFERENCE_STRING_SIZE] = "";

		DWORD numberOfBytesRead = 0;

		if (!ReadFile(file, preferenceString, PREFERENCE_STRING_SIZE - 1, &numberOfBytesRead, NULL)) {
			return false;
		}

		if (!preferenceStringToLong(preferenceString, preference)) {
			return false;
		}
	}
	return result;
}

bool getEnvironmentVariablePreference(LPCSTR environmentVariableName, long &preference) {
	preference = PREFERENCE_DEFAULT;

	if (!environmentVariableName) {
		return false;
	}

	const DWORD PREFERENCE_STRING_SIZE = 16;
	CHAR preferenceString[PREFERENCE_STRING_SIZE] = "";

	if (GetEnvironmentVariable(environmentVariableName, preferenceString, PREFERENCE_STRING_SIZE - 1)) {
		if (!preferenceStringToLong(preferenceString, preference)) {
			return false;
		}
	}
	return true;
}

bool getPreference(LPCSTR fileName, LPCSTR environmentVariableName, long &preference) {
	preference = PREFERENCE_DEFAULT;

	if (!fileName) {
		return false;
	}

	if (!environmentVariableName) {
		return false;
	}

	if (!getFilePreference(fileName, preference)) {
		return false;
	}

	if (preference == PREFERENCE_DEFAULT) {
		if (!getEnvironmentVariablePreference(environmentVariableName, preference)) {
			return false;
		}
	}
	return true;
}