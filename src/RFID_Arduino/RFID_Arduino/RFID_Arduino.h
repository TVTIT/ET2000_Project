#pragma once
#include <string>
#define VERSION_LABEL L"v1.0.0-stable"
using namespace std;

string GetTimeNow(int type);
void ClearScreen();
void PauseAndExit();
void PauseAndBack();
string UnicodeInput();
vector<string> SplitString(string input, char pattern);