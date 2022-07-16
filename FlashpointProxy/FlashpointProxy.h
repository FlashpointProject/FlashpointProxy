#pragma once
#include "shared.h"
#include <windows.h>
#include <WinInet.h>

class FlashpointProxy {
	private:
	static bool getSystemProxy(INTERNET_PER_CONN_OPTION_LIST &internetPerConnOptionList, DWORD internetPerConnOptionListOptionsSize);
	static LPCSTR AGENT;
	public:
	static bool enable(LPSTR proxyServer);
	static bool disable();
};