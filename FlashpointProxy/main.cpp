#include "shared.h"
#include "FlashpointProxy.h"
#include <windows.h>
#include <winnt.h>
#include <process.h>

LPSTR proxyServer = "http=127.0.0.1:22500;https=127.0.0.1:22500;ftp=127.0.0.1:22500";
CONTEXT originalMainThreadContext = {};

void entryPoint() {
	if (!FlashpointProxy::enable(proxyServer)) {
		showLastError("Failed to Enable Flashpoint Proxy");
		terminateCurrentProcess();
		return;
	}

	setCurrentThreadContext(originalMainThreadContext);
}

unsigned int __stdcall dynamicThread(void* argList) {
	if (!FlashpointProxy::enable(proxyServer)) {
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
		if (!DisableThreadLibraryCalls(instance)) {
			showLastError("Failed to Disable Thread Library Calls");
			terminateCurrentProcess();
			return FALSE;
		}

		// dynamic link
		if (!contextPointer) {
			if (!FlashpointProxy::enable(proxyServer)) {
				if (GetLastError() != ERROR_POSSIBLE_DEADLOCK) {
					showLastError("Failed to Enable Flashpoint Proxy");
					terminateCurrentProcess();
					return FALSE;
				}

				// deadlock fallback
				// nothing we can do except begin a thread
				// and hope it finishes before WinInet is used
				HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, dynamicThread, NULL, 0, NULL);

				if (!threadHandle || threadHandle == INVALID_HANDLE_VALUE) {
					showLastError("Failed to Begin Thread");
					terminateCurrentProcess();
					return FALSE;
				}

				if (!CloseHandle(threadHandle)) {
					showLastError("Failed to Close Handle");
					terminateCurrentProcess();
					return FALSE;
				}

				threadHandle = NULL;
			}
			return TRUE;
		}

		// static link
		// copy...
		originalMainThreadContext = *contextPointer;
		// modify original...
		#if defined _AMD64_ || defined _IA64_
		contextPointer->Rip = (DWORD64)entryPoint;
		#else
		contextPointer->Eip = (DWORD)entryPoint;
		#endif
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