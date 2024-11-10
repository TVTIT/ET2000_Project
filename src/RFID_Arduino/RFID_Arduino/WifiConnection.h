#pragma once

using namespace std;

class WifiConnection
{
public:
	static void Connect(string IP);
private:
	static bool isContinue;
	static void HookEnter();
};

