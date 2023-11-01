#pragma once
#include "shared.h"
#include <windows.h>
#include <WinInet.h>

class FlashpointProxy {
	private:
	static LPCSTR AGENT;

	static const bool FP_PROXY_DEFAULT;
	static const int FP_PROXY_PORT_DEFAULT;

	static LPCSTR FP_PROXY_FILE_NAME;
	static LPCSTR FP_PROXY_PORT_FILE_NAME;

	static LPCSTR FP_PROXY;
	static LPCSTR FP_PROXY_PORT;

	static bool getSystemProxy(INTERNET_PER_CONN_OPTION_LIST &internetPerConnOptionList, DWORD internetPerConnOptionListOptionsSize);

	public:
	static bool getPreferences(bool &proxy, int &port);
	static bool enable();
	static bool disable();
};