#pragma once
#include <string>
using namespace std;

string GetTimeNow(int type);
void ClearScreen();
void PauseAndExit();
void PauseAndBack();
string UnicodeInput();
vector<string> SplitString(string input, char pattern);