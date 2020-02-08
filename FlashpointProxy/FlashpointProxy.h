#pragma once
#include <windows.h>
#include <WinInet.h>
#include <string>

class FlashpointProxy {
	private:
	static bool getSystemProxy(INTERNET_PER_CONN_OPTION_LIST &internetPerConnOptionList, DWORD internetPerConnOptionListOptionsSize);
	static const LPCSTR AGENT;
	public:
	static bool enable(std::string proxyServer);
	static bool enable();
	static bool disable();
};