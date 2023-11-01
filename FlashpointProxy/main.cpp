#include "shared.h"
#include "FlashpointProxy.h"
#include <windows.h>
#include <winnt.h>
#include <process.h>

CONTEXT originalMainThreadContext = {};

void entryPoint() {
	try {
		if (!FlashpointProxy::enable()) {
			showLastError("Failed to Enable Flashpoint Proxy");
			terminateCurrentProcess();
			return;
		}
	} catch (...) {
		// the thread hasn't set up vectored exception handling at this stage
		terminateCurrentProcess();
		return;
	}

	setCurrentThreadContext(originalMainThreadContext);
}

unsigned int __stdcall dynamicThread(void* argList) {
	// do not create any C++ objects here
	// (unrolling interferes with _endthreadex)
	if (!FlashpointProxy::enable()) {
		showLastError("Failed to Enable Flashpoint Proxy");
		terminateCurrentProcess();
		_endthreadex(1);
		return 1;
	}

	_endthreadex(0);
	return 0;
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

		// static link
		if (contextPointer) {
			if (IS_INTRESOURCE(contextPointer)) {
				showLastError("Context Pointer invalid");
				return FALSE;
			}

			// copy...
			originalMainThreadContext = *contextPointer;
			// modify original...
			#if defined _AMD64_ || defined _IA64_
			contextPointer->Rip = (DWORD64)entryPoint;
			#else
			contextPointer->Eip = (DWORD)entryPoint;
			#endif

			terminateCurrentProcessScopeExit.dismiss();
			return TRUE;
		}

		// dynamic link
		if (!FlashpointProxy::enable()) {
			if (GetLastError() != ERROR_POSSIBLE_DEADLOCK) {
				showLastError("Failed to Enable Flashpoint Proxy");
				return FALSE;
			}

			BOOL result = TRUE;

			{
				// deadlock fallback
				// nothing we can do except begin a thread
				// and hope it finishes before WinInet is used
				HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, dynamicThread, NULL, 0, NULL);

				SCOPE_EXIT {
					if (closeThread(thread)) {
						result = FALSE;
					}
				};

				if (!thread || thread == INVALID_HANDLE_VALUE) {
					showLastError("Failed to Begin Thread");
					return FALSE;
				}
			}

			if (result) {
				terminateCurrentProcessScopeExit.dismiss();
			}
			return result;
		}

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