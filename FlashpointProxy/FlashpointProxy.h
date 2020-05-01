#pragma once
#include <windows.h>
#include <WinInet.h>
#include <string>

class FlashpointProxy {
	private:
	static bool getSystemProxy(INTERNET_PER_CONN_OPTION_LIST &internetPerConnOptionList, DWORD internetPerConnOptionListOptionsSize);
	static LPCSTR AGENT;
	public:
	static bool enable(std::string proxyServer);
	static bool disable();
};