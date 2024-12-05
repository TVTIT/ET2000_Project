#pragma once
#include <string>
using namespace std;

class ReadWriteCSV
{
public:
	static string DirectoryPath;
	static void GetStudentName(string id_card, string& student_name, bool& available);
	static void InitializeCSV();
	static void GetPath();
	static void InKetQuaDiemDanh();
	static void LuuDuLieuDiemDanh();
	static void AddStudentToCSV(string ID_Card);
	static void RemoveStudentFromCSV();
	static string GetStudentIDsCSV();
};