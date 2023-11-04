#include "FlashpointProxy.h"
#include <string>
#include <sstream>
#include <windows.h>
#include <WinInet.h>

LPCSTR FlashpointProxy::AGENT = "Flashpoint Proxy";

const bool FlashpointProxy::FP_PROXY_DEFAULT = true;
const int FlashpointProxy::FP_PROXY_PORT_DEFAULT = 22500;

LPCSTR FlashpointProxy::FP_PROXY_FILE_NAME = "proxy.txt";
LPCSTR FlashpointProxy::FP_PROXY_PORT_FILE_NAME = "port.txt";

bool FlashpointProxy::getSystemProxy(INTERNET_PER_CONN_OPTION_LIST &internetPerConnOptionList, DWORD internetPerConnOptionListOptionsSize) {
	if (!internetPerConnOptionList.pOptions) {
		showLastError("Options Pointer must not be NULL");
		return false;
	}

	if (internetPerConnOptionListOptionsSize < 2) {
		showLastError("internetPerConnOptionListOptionsSize must not be less than 2");
		return false;
	}

	// set flags
	internetPerConnOptionList.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;

	// set proxy name
	internetPerConnOptionList.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;

	DWORD internetPerConnOptionListSize = sizeof(internetPerConnOptionList);

	// fill the internetPerConnOptionList structure
	internetPerConnOptionList.dwSize = internetPerConnOptionListSize;

	// NULL == LAN, otherwise connectoid name
	internetPerConnOptionList.pszConnection = NULL;

	// set two options
	internetPerConnOptionList.dwOptionCount = internetPerConnOptionListOptionsSize;
	internetPerConnOptionList.dwOptionError = 0;

	// query internet options
	if (!InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &internetPerConnOptionList, &internetPerConnOptionListSize)) {
		showLastError("Failed to Query Internet Option");
		return false;
	}
	return true;
}

LPCSTR FlashpointProxy::FP_PROXY = "FP_PROXY";
LPCSTR FlashpointProxy::FP_PROXY_PORT = "FP_PROXY_PORT";

bool FlashpointProxy::getPreferences(bool &proxy, int &port) {
	proxy = FP_PROXY_DEFAULT;
	port = FP_PROXY_PORT_DEFAULT;

	long preference = PREFERENCE_DEFAULT;
	
	if (!getPreference(FP_PROXY_FILE_NAME, FP_PROXY, preference)) {
		showLastError("Failed to Get Preference");
		return false;
	}

	if (preference != PREFERENCE_DEFAULT) {
		proxy = (bool)preference;
	}

	preference = PREFERENCE_DEFAULT;

	if (!getPreference(FP_PROXY_PORT_FILE_NAME, FP_PROXY_PORT, preference)) {
		showLastError("Failed to Get Preference");
		return false;
	}

	if (preference != PREFERENCE_DEFAULT) {
		port = preference;
	}
	return true;
}

bool FlashpointProxy::enable() {
	bool proxy = FP_PROXY_DEFAULT;
	int port = FP_PROXY_PORT_DEFAULT;

	if (!getPreferences(proxy, port)) {
		showLastError("Failed to Get Preferences");
		return false;
	}

	if (!proxy) {
		return true;
	}

	bool result = true;

	std::ostringstream oStringStream;
	oStringStream << "http=127.0.0.1:" << port << ";https=127.0.0.1:" << port << ";ftp=127.0.0.1:" << port;

	std::string proxyServer = oStringStream.str();

	HINTERNET internetHandle = InternetOpen(AGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

	SCOPE_EXIT {
		DWORD lastError = GetLastError();

		if (!closeInternetHandle(internetHandle)) {
			showLastError("Failed to Close Internet Handle");
			result = false;
		}

		if (lastError) {
			SetLastError(lastError);
		}
	};

	if (!internetHandle) {
		showLastError("Failed to Open Internet Handle");
		return false;
	}

	// create two options
	const DWORD INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE = 2;
	std::unique_ptr<INTERNET_PER_CONN_OPTION[]> optionsPointer = std::unique_ptr<INTERNET_PER_CONN_OPTION[]>(new INTERNET_PER_CONN_OPTION[INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE]);

	if (!optionsPointer) {
		showLastError("Failed to Allocate optionsPointer");
		return false;
	}

	// set PROXY flags
	optionsPointer[0].dwOption = INTERNET_PER_CONN_FLAGS;
	optionsPointer[0].Value.dwValue = PROXY_TYPE_PROXY;

	// set proxy name
	optionsPointer[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;

	size_t proxyServerSize = proxyServer.size() + 1;
	std::unique_ptr<CHAR[]> _proxyServer = std::unique_ptr<CHAR[]>(new CHAR[proxyServerSize]);

	if (!_proxyServer) {
		showLastError("Failed to Allocate proxyServer");
		return false;
	}

	if (strncpy_s(_proxyServer.get(), proxyServerSize, proxyServer.c_str(), proxyServerSize)) {
		showLastError("Failed to Copy String");
		return false;
	}

	optionsPointer[1].Value.pszValue = _proxyServer.get();

	if (!optionsPointer[1].Value.pszValue) {
		showLastError("Value Pointer must not be NULL");
		return false;
	}

	// initialize a INTERNET_PER_CONN_OPTION_LIST instance
	INTERNET_PER_CONN_OPTION_LIST internetPerConnOptionList = {};
	const DWORD INTERNET_PER_CONN_OPTION_LIST_SIZE = sizeof(internetPerConnOptionList);

	internetPerConnOptionList.pOptions = optionsPointer.get();

	if (!internetPerConnOptionList.pOptions) {
		showLastError("Options Pointer must not be NULL");
		return false;
	}

	// fill the internetPerConnOptionList structure
	internetPerConnOptionList.dwSize = INTERNET_PER_CONN_OPTION_LIST_SIZE;

	// NULL == LAN, otherwise connectoid name
	internetPerConnOptionList.pszConnection = NULL;

	// set two options
	internetPerConnOptionList.dwOptionCount = INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE;
	internetPerConnOptionList.dwOptionError = 0;

	// set the options on the connection
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_PER_CONNECTION_OPTION, &internetPerConnOptionList, INTERNET_PER_CONN_OPTION_LIST_SIZE)) {
		showLastError("Failed to Set Internet Option");
		return false;
	}
	return result;
}

bool FlashpointProxy::disable() {
	bool result = true;

	HINTERNET internetHandle = InternetOpen(AGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

	SCOPE_EXIT {
		DWORD lastError = GetLastError();

		if (!closeInternetHandle(internetHandle)) {
			showLastError("Failed to Close Internet Handle");
			result = false;
		}

		if (lastError) {
			SetLastError(lastError);
		}
	};

	if (!internetHandle) {
		showLastError("Failed to Open Internet Handle");
		return false;
	}

	// create two options
	const DWORD INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE = 2;
	std::unique_ptr<INTERNET_PER_CONN_OPTION[]> optionsPointer = std::unique_ptr<INTERNET_PER_CONN_OPTION[]>(new INTERNET_PER_CONN_OPTION[INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE]);

	if (!optionsPointer) {
		showLastError("Failed to Allocate optionsPointer");
		return false;
	}

	// initialize a INTERNET_PER_CONN_OPTION_LIST instance
	INTERNET_PER_CONN_OPTION_LIST internetPerConnOptionList = {};
	const DWORD internetPerConnOptionListSize = sizeof(internetPerConnOptionList);

	internetPerConnOptionList.pOptions = optionsPointer.get();

	if (!internetPerConnOptionList.pOptions) {
		showLastError("Options Pointer must not be NULL");
		return false;
	}
	
	if (!getSystemProxy(internetPerConnOptionList, INTERNET_PER_CONN_OPTION_LIST_OPTIONS_SIZE)) {
		showLastError("Failed to Get System Proxy");
		return false;
	}
	
    // set internet options
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_PER_CONNECTION_OPTION, &internetPerConnOptionList, internetPerConnOptionListSize)) {
		showLastError("Failed to Set Internet Option");
		return false;
	}
	
	// notify the system that the registry settings have been changed and cause
    // the proxy data to be reread from the registry for a handle
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0)) {
		showLastError("Failed to Set Internet Option");
		return false;
	}
	
	if (!InternetSetOption(internetHandle, INTERNET_OPTION_REFRESH, NULL, 0)) {
		showLastError("Failed to Set Internet Option");
		return false;
	}
	return result;
}