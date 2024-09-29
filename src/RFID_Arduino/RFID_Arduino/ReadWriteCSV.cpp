#include "rapidcsv.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include <fmt/base.h>
#include <fmt/chrono.h>
#include <fcntl.h>
#include <io.h>

#include "RFID_Arduino.h"
#include "ReadWriteCSV.h"

using namespace std;
using namespace rapidcsv;

vector<string> v_students_names;
vector<string> v_students_IDscard;
vector<string> v_students_isPresent;

string ReadWriteCSV::DirectoryPath;
int studentCount;

/// <summary>
/// Convert từ wstring sang string (utf8) để lưu tên SV vào file CSV
/// </summary>
/// <param name="wstr">wstring cần convert</param>
/// <returns></returns>
string wstring_to_utf8(const wstring& wstr) 
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    string str(size_needed - 1, 0);  // Trừ đi 1 để bỏ ký tự null-terminator
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size_needed, nullptr, nullptr);
    return str;
}

/// <summary>
/// Lấy tên + MSSV của SV qua mã thẻ RFID
/// </summary>
/// <param name="id_card">ID thẻ</param>
/// <param name="student_name">Biến chứa tên SV để trả về</param>
/// <param name="available">Biến trả về sinh viên có tồn tại hay không</param>
void ReadWriteCSV::GetStudentName(string id_card, string& student_name, bool& available)
{
    for (size_t i = 0; i < studentCount; i++)
    {
        if (v_students_IDscard[i] == id_card)
        {
            student_name = v_students_names[i];
            v_students_isPresent[i] = "x";
            available = true;
            return;
        }
    }
    available = false;
}

/// <summary>
/// Khởi tạo đọc file CSV, tạo các vector cần thiết
/// </summary>
void ReadWriteCSV::InitializeCSV()
{
    rapidcsv::Document doc(ReadWriteCSV::DirectoryPath + "\\students_list.csv", rapidcsv::LabelParams(0, -1));

    v_students_names = doc.GetColumn<string>("Name and Student ID");
    v_students_IDscard = doc.GetColumn<string>("ID card");

    studentCount = v_students_IDscard.size();

    v_students_isPresent.resize(studentCount);
}

/// <summary>
/// Lấy đường dẫn file exe, chỉ cần gọi 1 lần ở hàm main()
/// </summary>
void ReadWriteCSV::GetPath()
{
    char PathWithExe[MAX_PATH];
    GetModuleFileNameA(NULL, PathWithExe, MAX_PATH);
    string str_PathWithExe(PathWithExe);
    ReadWriteCSV::DirectoryPath = str_PathWithExe.substr(0, str_PathWithExe.find_last_of("\\/"));
}

/// <summary>
/// In kết quả điểm danh (SV có mặt/vắng) ra console
/// </summary>
void ReadWriteCSV::InKetQuaDiemDanh()
{
    string StudentsPresent;
    string StudentsNOTPresent;
    for (size_t i = 0; i < studentCount; i++)
    {
        if (v_students_isPresent[i] == "x")
            StudentsPresent += fmt::format("{0}\n", v_students_names[i]);
        else
            StudentsNOTPresent += fmt::format("{0}\n", v_students_names[i]);
    }
    fmt::println("Danh sách sinh viên có mặt:\n" + StudentsPresent + "\n\n");
    fmt::println("Danh sách sinh viên KHÔNG có mặt:\n" + StudentsNOTPresent + "\n\n");
}

/// <summary>
/// Xuất dữ liệu điểm danh ra file CSV
/// </summary>
void ReadWriteCSV::LuuDuLieuDiemDanh()
{
    rapidcsv::Document doc(ReadWriteCSV::DirectoryPath + "\\students_list.csv", rapidcsv::LabelParams(0, -1));
    doc.InsertColumn(2, v_students_isPresent, "Is present");
    doc.Save(ReadWriteCSV::DirectoryPath + "\\Du lieu diem danh " + GetTimeNow(1) + ".csv");

    v_students_isPresent.clear();
    v_students_isPresent.resize(studentCount);

    fmt::println("Đã lưu dữ liệu điểm danh vào file csv!");
}

/// <summary>
/// Thêm sinh viên vào file CSV
/// </summary>
/// <param name="ID_Card">ID thẻ vừa quét được</param>
void ReadWriteCSV::AddStudentToCSV(string ID_Card)
{
    fmt::println("ID thẻ vừa quét: " + ID_Card);
    fmt::print("Nhập tên+ MSSV của sinh viên vừa quét: ");

    /* Đặt chế độ input UTF18 để nhập tên tiếng Việt có dấu, sau đó đặt lại
    chế độ ban đầu để tránh xung đột */
    _setmode(_fileno(stdin), _O_U16TEXT);
    wstring wstr_student_name;
    getline(wcin, wstr_student_name);
    _setmode(_fileno(stdin), _O_TEXT);

    string student_name = wstring_to_utf8(wstr_student_name);

    vector<string> newInfo = { student_name, ID_Card };
    rapidcsv::Document doc(ReadWriteCSV::DirectoryPath + "\\students_list.csv", rapidcsv::LabelParams(0, -1));
    doc.InsertRow(studentCount, newInfo);
    doc.Save();

    InitializeCSV();

    fmt::println("Lưu thành công thông tin sinh viên!");
}
