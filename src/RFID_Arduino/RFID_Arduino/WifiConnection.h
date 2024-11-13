#pragma once

using namespace std;

class WifiConnection
{
public:
	static void Connect(string IP);
private:
	static bool isContinue;
	static bool lastConnectionError;
	static void HookEnter();
	static void Reconnect(string IP);
};

