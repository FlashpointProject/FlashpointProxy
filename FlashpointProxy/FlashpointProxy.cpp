#include "FlashpointProxy.h"
#include <windows.h>
#include <WinInet.h>
#include <process.h>
#include <string>

LPCSTR FlashpointProxy::AGENT = "Flashpoint Proxy";

const std::string PROXY_SERVER = "http=127.0.0.1:22500;https=127.0.0.1:22500;ftp=127.0.0.1:22500";
DWORD mainThreadID = 0;
HANDLE entryPointEventHandle = INVALID_HANDLE_VALUE;
CONTEXT originalMainThreadContext;




inline size_t stringSize(const char* string) {
	return strlen(string) + 1;
}








bool FlashpointProxy::getSystemProxy(INTERNET_PER_CONN_OPTION_LIST &internetPerConnOptionList, DWORD internetPerConnOptionListOptionsSize) {
	DWORD internetPerConnOptionListSize = sizeof(internetPerConnOptionList);

	if (!internetPerConnOptionList.pOptions) {
		return false;
	}

	if (internetPerConnOptionListOptionsSize < 2) {
		return false;
	}

	// set flags
	internetPerConnOptionList.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;

	// set proxy name
	internetPerConnOptionList.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;

	// fill the internetPerConnOptionList structure
	internetPerConnOptionList.dwSize = internetPerConnOptionListSize;

	// NULL == LAN, otherwise connectoid name
	internetPerConnOptionList.pszConnection = NULL;

	// set two options
	internetPerConnOptionList.dwOptionCount = internetPerConnOptionListOptionsSize;
	internetPerConnOptionList.dwOptionError = 0;

	// query internet options
	if (!InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &internetPerConnOptionList, &internetPerConnOptionListSize)) {
		return false;
	}
	return true;
}

bool FlashpointProxy::enable(std::string proxyServer) {
	HINTERNET internetHandle = InternetOpen(AGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	
    // initialize a INTERNET_PER_CONN_OPTION_LIST instance
	INTERNET_PER_CONN_OPTION_LIST internetPerConnOptionList;
	const DWORD internetPerConnOptionListSize = sizeof(internetPerConnOptionList);

	// create two options
	const DWORD INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE = 2;
	internetPerConnOptionList.pOptions = new INTERNET_PER_CONN_OPTION[INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE];

	if (!internetPerConnOptionList.pOptions) {
		InternetCloseHandle(internetHandle);
		return false;
	}

	// set PROXY flags
	internetPerConnOptionList.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
	internetPerConnOptionList.pOptions[0].Value.dwValue = PROXY_TYPE_PROXY;

	// set proxy name
	internetPerConnOptionList.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
	size_t proxyServerSize = proxyServer.length() + 1;
	internetPerConnOptionList.pOptions[1].Value.pszValue = new CHAR[proxyServerSize];

	if (!internetPerConnOptionList.pOptions[1].Value.pszValue) {
		if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}

		InternetCloseHandle(internetHandle);
		return false;
	}

	if (strncpy_s(internetPerConnOptionList.pOptions[1].Value.pszValue, proxyServerSize, proxyServer.c_str(), proxyServerSize)) {
		if (internetPerConnOptionList.pOptions[1].Value.pszValue) {
			delete[] internetPerConnOptionList.pOptions[1].Value.pszValue;
			internetPerConnOptionList.pOptions[1].Value.pszValue = NULL;
		}

		if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}

		InternetCloseHandle(internetHandle);
		return false;
	}

	// fill the internetPerConnOptionList structure
	internetPerConnOptionList.dwSize = internetPerConnOptionListSize;

	// NULL == LAN, otherwise connectoid name
	internetPerConnOptionList.pszConnection = NULL;

	// set two options
	internetPerConnOptionList.dwOptionCount = INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE;
	internetPerConnOptionList.dwOptionError = 0;

	// set the options on the connection
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_PER_CONNECTION_OPTION, &internetPerConnOptionList, internetPerConnOptionListSize)) {
        // free the allocated memory
		if (internetPerConnOptionList.pOptions[1].Value.pszValue) {
			delete[] internetPerConnOptionList.pOptions[1].Value.pszValue;
			internetPerConnOptionList.pOptions[1].Value.pszValue = NULL;
		}

		if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}
		
		InternetCloseHandle(internetHandle);
		return false;
	}
	
	// free the allocated memory
	if (internetPerConnOptionList.pOptions[1].Value.pszValue) {
		delete[] internetPerConnOptionList.pOptions[1].Value.pszValue;
		internetPerConnOptionList.pOptions[1].Value.pszValue = NULL;
	}
	
	if (internetPerConnOptionList.pOptions) {
		delete[] internetPerConnOptionList.pOptions;
		internetPerConnOptionList.pOptions = NULL;
	}
	
	if (!InternetCloseHandle(internetHandle)) {
		return false;
	}
	return true;
}

bool FlashpointProxy::disable() {
	HINTERNET internetHandle = InternetOpen(AGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	
    // initialize a INTERNET_PER_CONN_OPTION_LIST instance
	INTERNET_PER_CONN_OPTION_LIST internetPerConnOptionList;
	const DWORD internetPerConnOptionListSize = sizeof(internetPerConnOptionList);

	// create two options
	const DWORD INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE = 2;
	internetPerConnOptionList.pOptions = new INTERNET_PER_CONN_OPTION[INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE];

	if (!internetPerConnOptionList.pOptions) {
		return false;
	}
	
	if (!getSystemProxy(internetPerConnOptionList, INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE)) {
		InternetCloseHandle(internetHandle);
		return false;
	}
	
    // set internet options
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_PER_CONNECTION_OPTION, &internetPerConnOptionList, internetPerConnOptionListSize)) {
        if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}
		
		InternetCloseHandle(internetHandle);
		return false;
	}
	
	// notify the system that the registry settings have been changed and cause
    // the proxy data to be reread from the registry for a handle
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0)) {
        if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}
		
		InternetCloseHandle(internetHandle);
		return false;
	}
	
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_REFRESH, NULL, 0)) {
        if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}
		
		InternetCloseHandle(internetHandle);
		return false;
	}
	
	if (!InternetCloseHandle(internetHandle)) {
		return false;
	}
	
	if (internetPerConnOptionList.pOptions) {
		delete[] internetPerConnOptionList.pOptions;
		internetPerConnOptionList.pOptions = NULL;
	}
	return true;
}








/*
void link() {
	MenuHelp(); // COMCTL32
	SHChangeNotifyRegister(); // SHELL32
	WahCloseApcHelper(); // WS2HELP
	accept(); // WS2_32
	bind(); // WSOCK32
	WinHttpPacJsWorkerMain(); // WINHTTP
}
*/

bool showLastError(LPCSTR errorMessage) {
	if (!errorMessage) {
		return false;
	}

	DWORD lastError = GetLastError();
	SIZE_T textSize = stringSize(errorMessage) + 15;
	LPTSTR text = new CHAR[textSize];

	if (sprintf_s(text, textSize, "%s (%d)", errorMessage, lastError) == -1) {
		delete[] text;
		text = NULL;
		return false;
	}

	if (!MessageBox(NULL, text, "Flashpoint Proxy", MB_OK | MB_ICONERROR)) {
		delete[] text;
		text = NULL;
		return false;
	}

	delete[] text;
	text = NULL;
	return true;
}

unsigned int __stdcall thread(HANDLE entryPointEventHandle) {
	if (!FlashpointProxy::enable(PROXY_SERVER)) {
		showLastError("Failed to Enable Flashpoint Proxy");
		TerminateProcess(GetCurrentProcess(), -1);
		return 1;
	}




	if (!entryPointEventHandle || entryPointEventHandle == INVALID_HANDLE_VALUE) {
		showLastError("Entry Point Event Handle cannot be NULL or INVALID_HANDLE_VALUE");
		TerminateProcess(GetCurrentProcess(), -1);
		return 1;
	}

	// wait for event from entry point
	if (WaitForSingleObject(entryPointEventHandle, INFINITE) != WAIT_OBJECT_0) {
		showLastError("Failed to Wait For Entry Point Event Single Object");
		TerminateProcess(GetCurrentProcess(), -1);
		return 1;
	}

	if (!CloseHandle(entryPointEventHandle)) {
		showLastError("Failed to Close Entry Point Event Handle");
		TerminateProcess(GetCurrentProcess(), -1);
		return 1;
	}

	entryPointEventHandle = INVALID_HANDLE_VALUE;
	HANDLE mainThreadHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_SET_CONTEXT, FALSE, mainThreadID);

	if (!mainThreadHandle || mainThreadHandle == INVALID_HANDLE_VALUE) {
		showLastError("Failed to Open Main Thread");
		TerminateProcess(GetCurrentProcess(), -1);
		return 1;
	}

	if (SuspendThread(mainThreadHandle) == -1) {
		showLastError("Failed to Suspend Main Thread");
		TerminateProcess(GetCurrentProcess(), -1);
		return 1;
	}

	if (!SetThreadContext(mainThreadHandle, &originalMainThreadContext)) {
		showLastError("Failed to Set Main Thread Context");
		TerminateProcess(GetCurrentProcess(), -1);
		return 1;
	}

	if (ResumeThread(mainThreadHandle) == -1) {
		showLastError("Failed to Resume Main Thread");
		TerminateProcess(GetCurrentProcess(), -1);
		return 1;
	}

	if (!CloseHandle(mainThreadHandle)) {
		showLastError("Failed to Close Main Thread Handle");
		TerminateProcess(GetCurrentProcess(), -1);
		return 1;
	}

	mainThreadHandle = INVALID_HANDLE_VALUE;
	return 0;
}

void entryPoint() {
	mainThreadID = GetCurrentThreadId();

	if (!entryPointEventHandle || entryPointEventHandle == INVALID_HANDLE_VALUE) {
		TerminateProcess(GetCurrentProcess(), -2);
		while (true) {}
	}

	if (!SetEvent(entryPointEventHandle)) {
		TerminateProcess(GetCurrentProcess(), -3);
	}

	while (true) {}
}

extern "C" BOOL APIENTRY DllMain(HMODULE moduleHandle, DWORD fdwReason, PCONTEXT contextPointer) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(moduleHandle);




		// dynamic link
		if (!contextPointer) {
			if (!FlashpointProxy::enable(PROXY_SERVER)) {
				showLastError("Failed to Enable Flashpoint Proxy");
				TerminateProcess(GetCurrentProcess(), -1);
				return FALSE;
			}
			return TRUE;
		}








		// static link
		// synchronization event (safe to create in DllMain)
		entryPointEventHandle = CreateEvent(NULL, TRUE, FALSE, "Flashpoint Proxy Entry Point Event");

		if (!entryPointEventHandle || entryPointEventHandle == INVALID_HANDLE_VALUE) {
			showLastError("Failed to Create Entry Point Event");
			TerminateProcess(GetCurrentProcess(), -1);
			return FALSE;
		}




		// step one: entry point
		// copy...
		originalMainThreadContext = *contextPointer;
		// modify original...
		contextPointer->Eip = (DWORD)entryPoint;




		// step two: thread (can be created in DllMain, synchronization offloaded to entry point)
		HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, thread, entryPointEventHandle, 0, 0);

		if (!threadHandle || threadHandle == INVALID_HANDLE_VALUE) {
			showLastError("Failed to Begin Thread Ex");
			TerminateProcess(GetCurrentProcess(), -1);
			return FALSE;
		}

		if (!CloseHandle(threadHandle)) {
			showLastError("Failed to Close Thread Handle");
			TerminateProcess(GetCurrentProcess(), -1);
			return FALSE;
		}

		threadHandle = INVALID_HANDLE_VALUE;
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