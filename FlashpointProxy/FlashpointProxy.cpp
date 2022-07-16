#include "FlashpointProxy.h"
#include <windows.h>
#include <WinInet.h>

LPCSTR FlashpointProxy::AGENT = "Flashpoint Proxy";

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

bool FlashpointProxy::enable(LPSTR proxyServer) {
	DWORD lastError = 0;
	HINTERNET internetHandle = InternetOpen(AGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

	if (!internetHandle) {
		return false;
	}
	
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
	size_t proxyServerSize = stringSize(proxyServer);
	internetPerConnOptionList.pOptions[1].Value.pszValue = new CHAR[proxyServerSize];

	if (!internetPerConnOptionList.pOptions[1].Value.pszValue) {
		if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}

		InternetCloseHandle(internetHandle);
		return false;
	}

	if (strncpy_s(internetPerConnOptionList.pOptions[1].Value.pszValue, proxyServerSize, proxyServer, proxyServerSize)) {
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
		lastError = GetLastError();

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
		SetLastError(lastError);
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
	DWORD lastError = 0;
	HINTERNET internetHandle = InternetOpen(AGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

	if (!internetHandle) {
		return false;
	}
	
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
		lastError = GetLastError();

        if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}
		
		InternetCloseHandle(internetHandle);
		SetLastError(lastError);
		return false;
	}
	
	// notify the system that the registry settings have been changed and cause
    // the proxy data to be reread from the registry for a handle
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0)) {
		lastError = GetLastError();

        if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}
		
		InternetCloseHandle(internetHandle);
		SetLastError(lastError);
		return false;
	}
	
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_REFRESH, NULL, 0)) {
		lastError = GetLastError();

        if (internetPerConnOptionList.pOptions) {
			delete[] internetPerConnOptionList.pOptions;
			internetPerConnOptionList.pOptions = NULL;
		}
		
		InternetCloseHandle(internetHandle);
		SetLastError(lastError);
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