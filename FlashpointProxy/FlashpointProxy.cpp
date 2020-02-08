#include "FlashpointProxy.h"
#include <windows.h>
#include <WinInet.h>
#include <string>

const LPCSTR FlashpointProxy::AGENT = "Flashpoint Proxy";

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

extern "C" BOOL APIENTRY DllMain(HMODULE moduleHandle, DWORD fdwReason, LPVOID lpReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(moduleHandle);

		{
			if (!FlashpointProxy::enable("http=127.0.0.1:22500;https=127.0.0.1:22500;ftp=127.0.0.1:22500")) {
				MessageBox(NULL, "Connection could not be established with the remote host.", "Flashpoint Proxy", MB_OK | MB_ICONERROR);
				TerminateProcess(GetCurrentProcess(), -1);
				return FALSE;
			}
		}
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