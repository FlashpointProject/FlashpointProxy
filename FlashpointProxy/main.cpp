#include "shared.h"
#include "FlashpointProxy.h"
#include <windows.h>
#include <winnt.h>
#include <process.h>

CONTEXT originalMainThreadContext = {};

// just to be safe, this is in it's own function
// I want this exception handler to be cleaned up
// before the call to setCurrentThreadContext
bool __declspec(noinline) enableFlashpointProxyNoExcept() noexcept {
	try {
		if (!FlashpointProxy::enable()) {
			return false;
		}
	} catch (...) {
		return false;
	}
	return true;
}

void entryPoint() {
	// the thread hasn't set up vectored exception handling at this stage
	if (!enableFlashpointProxyNoExcept()) {
		showLastError("Failed to Enable Flashpoint Proxy");
		terminateCurrentProcess();
		return;
	}

	setCurrentThreadContext(originalMainThreadContext);
}

unsigned int __stdcall dynamicThread(void* argList) {
	// do not create any C++ objects here
	// (unwinding interferes with _endthreadex)
	if (!FlashpointProxy::enable()) {
		showLastError("Failed to Enable Flashpoint Proxy");
		terminateCurrentProcess();
		_endthreadex(1);
		return 1;
	}

	_endthreadex(0);
	return 0;
}

bool dynamicLink() {
	if (!FlashpointProxy::enable()) {
		if (GetLastError() != ERROR_POSSIBLE_DEADLOCK) {
			showLastError("Failed to Enable Flashpoint Proxy");
			return false;
		}

		bool result = true;

		// deadlock fallback
		// nothing we can do except begin a thread
		// and hope it finishes before WinInet is used
		HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, dynamicThread, NULL, 0, NULL);

		SCOPE_EXIT {
			if (!closeThread(thread)) {
				showLastError("Failed to Close Thread");
				result = false;
			}
		};

		if (!thread || thread == INVALID_HANDLE_VALUE) {
			showLastError("Failed to Begin Thread");
			return false;
		}
		return result;
	}
	return true;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, PCONTEXT contextPointer) {
	if (reason == DLL_PROCESS_ATTACH) {
		MAKE_SCOPE_EXIT(terminateCurrentProcessScopeExit) {
			terminateCurrentProcess();
		};

		if (!DisableThreadLibraryCalls(instance)) {
			showLastError("Failed to Disable Thread Library Calls");
			return FALSE;
		}

		// check whether the context pointer actually pointing to anything
		if (IS_INTRESOURCE(contextPointer)) {
			// dynamic link
			if (!dynamicLink()) {
				showLastError("Failed to Dynamic Link");
				return FALSE;
			}

			terminateCurrentProcessScopeExit.dismiss();
			return TRUE;
		}

		// copy...
		originalMainThreadContext = *contextPointer;

		// set the new origin
		DWORD_PTR origin = (DWORD_PTR)entryPoint;

		// modify original...
		#if defined _AMD64_ || defined _IA64_
		contextPointer->Rip = origin;
		#else
		contextPointer->Eip = origin;
		#endif

		terminateCurrentProcessScopeExit.dismiss();
	}
	return TRUE;
}

/*
"The Lord said, 'Go out and stand on the mountain in the presence of the Lord, for the Lord is about to pass by.'

Then a great and powerful wind tore the mountains apart and shattered the rocks before the Lord, but the Lord was not in the wind.
After the wind there was an earthquake, but the Lord was not in the earthquake.
After the earthquake came a fire, but the Lord was not in the fire.
And after the fire came a gentle whisper." - 1 Kings 19:11-12
*/