#include "shared.h"
#include <windows.h>
#include <WinNT.h>
#include <string>
#include <sstream>

static LPCSTR messageBoxCaption = "Flashpoint Proxy";

bool showLastError(LPCSTR errorMessage) {
	if (!errorMessage) {
		return false;
	}

	std::ostringstream oStringStream;
	oStringStream << errorMessage << " (" << GetLastError() << ")";

	if (!MessageBox(NULL, oStringStream.str().c_str(), messageBoxCaption, MB_OK | MB_ICONERROR)) {
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

	// set the new thread context to the one passed in as an argument
	*ContextRecord = *(PCONTEXT)ExceptionRecord->ExceptionInformation[0];
	return ExceptionContinueExecution;
}
#endif

void setCurrentThreadContext(CONTEXT &context) {
	#if defined _AMD64_ || defined _IA64_
	// intended Windows function for this purpose, 64-bit only :(
	RtlRestoreContext(&context, NULL);
	#else
	// use SEH to set the current thread's context
	DWORD handler = (DWORD)_except_handler_safeseh;

	// pass the new thread context as an argument
	ULONG_PTR setCurrentThreadContextArguments[SET_CURRENT_THREAD_CONTEXT_NUMBER_OF_ARGUMENTS] = { (ULONG_PTR)&context };

	__asm {
		// build EXCEPTION_REGISTRATION record
		push handler // address of handler function
		push fs:[0] // address of previous handler
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