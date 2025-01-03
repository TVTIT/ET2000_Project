﻿#include "rapidcsv.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include "fmt/core.h"
#include "fmt/base.h"
#include "fmt/chrono.h"
#include "fmt/color.h"
#include <fcntl.h>
#include <io.h>

#include "RFID_Arduino.h"
#include "ReadWriteCSV.h"

using namespace std;
using namespace rapidcsv;

rapidcsv::Document Student_ListCSV;
vector<string> v_students_names;
vector<string> v_students_IDscard;
vector<string> v_students_isPresent;

string ReadWriteCSV::DirectoryPath;
int studentCount;

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
    ifstream file;
    file.open(ReadWriteCSV::DirectoryPath + "\\students_list.csv");
    if (!file)
    {
        CreateNewFile();
        Student_ListCSV = rapidcsv::Document(ReadWriteCSV::DirectoryPath + "\\students_list.csv", rapidcsv::LabelParams(0, -1));
        studentCount = 0;
        return;
    }
    file.close();

    Student_ListCSV = rapidcsv::Document(ReadWriteCSV::DirectoryPath + "\\students_list.csv", rapidcsv::LabelParams(0, -1));

    v_students_names = Student_ListCSV.GetColumn<string>(0);
    v_students_IDscard = Student_ListCSV.GetColumn<string>(1);

    studentCount = v_students_IDscard.size();

    v_students_isPresent.resize(studentCount + 1);
}

void ReadWriteCSV::CreateNewFile()
{
    rapidcsv::Document doc(string(), rapidcsv::LabelParams(-1, -1)); //không nhận cột, ô nào làm label
    vector<string> v = { "Name and Student ID","ID card" };
    doc.InsertRow<string>(0, v);

    doc.Save(ReadWriteCSV::DirectoryPath + "\\students_list.csv");
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

    int countStudentPresent = 0;
    int countStudentNOTPresent = 0;

    for (size_t i = 0; i < studentCount; i++)
    {
        if (v_students_isPresent[i] == "x")
        {
            StudentsPresent += v_students_names[i] + "\n";
            countStudentPresent++;
        }
        else
        {
            StudentsNOTPresent += v_students_names[i] + "\n";
            countStudentNOTPresent++;
        }
    }
    v_students_isPresent[studentCount] = "Count: " + to_string(countStudentPresent);

    fmt::println("Có {0} sinh viên có mặt. Danh sách sinh viên có mặt:\n{1}\n\n",countStudentPresent, StudentsPresent);
    fmt::println("Có {0} sinh viên KHÔNG có mặt. Danh sách sinh viên KHÔNG có mặt:\n{1}\n\n", countStudentNOTPresent, StudentsNOTPresent);
}

/// <summary>
/// Xuất dữ liệu điểm danh ra file CSV
/// </summary>
void ReadWriteCSV::LuuDuLieuDiemDanh()
{
    rapidcsv::Document doc = Student_ListCSV;
    doc.InsertColumn(2, v_students_isPresent, "Is present");
    doc.Save(ReadWriteCSV::DirectoryPath + "\\Du lieu diem danh " + GetTimeNow(1) + ".csv");
    doc.RemoveColumn(2);
    doc.RemoveRow(studentCount);

    v_students_isPresent.clear();
    v_students_isPresent.resize(studentCount + 1);

    fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), "Đã lưu dữ liệu điểm danh vào file csv!");
    fmt::println("");
}

/// <summary>
/// Thêm sinh viên vào file CSV
/// </summary>
/// <param name="ID_Card">ID thẻ vừa quét được</param>
void ReadWriteCSV::AddStudentToCSV(string ID_Card)
{
    fmt::print("ID thẻ vừa quét: ");
    fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::gray), ID_Card + "\n");
    fmt::print("Nhập tên + MSSV của sinh viên vừa quét: ");

    /* Đặt chế độ input UTF18 để nhập tên tiếng Việt có dấu, sau đó đặt lại
    chế độ ban đầu để tránh xung đột */
    //_setmode(_fileno(stdin), _O_U16TEXT);
    //wstring wstr_student_name;
    //getline(wcin, wstr_student_name);
    //_setmode(_fileno(stdin), _O_TEXT);

    //string student_name = wstring_to_utf8(wstr_student_name);

    string student_name = UnicodeInput();

    vector<string> newInfo = { student_name, ID_Card };
    Student_ListCSV.InsertRow(studentCount, newInfo);
    Student_ListCSV.Save();

    InitializeCSV();

    fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), "Lưu thành công thông tin sinh viên!\n");
}

/// <summary>
/// Xoá sinh viên khỏi file CSV
/// </summary>
void ReadWriteCSV::RemoveStudentFromCSV()
{
    ClearScreen();
    fmt::println("Danh sách sinh viên hiện tại:\n");
    
    for (int i = 0; i < studentCount; i++)
    {
        fmt::println("[{0}] {1}", i + 1, v_students_names[i]);
    }

    fmt::print("\nNhập số thứ tự sinh viên muốn xoá: ");
    string user_input;
    getline(cin, user_input);

    //Try catch đề phòng user nhập ký tự không phải số
    try
    {
        int int_user_input = stoi(user_input);
        //Kiểm tra xem số nhập vào có nằm trong số thứ tự không
        if (int_user_input <= studentCount)
        {
            Student_ListCSV.RemoveRow(int_user_input - 1);
            Student_ListCSV.Save();
            ReadWriteCSV::InitializeCSV();

            fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), "Xoá sinh viên thành công!");
            fmt::println("");
        }
        else
        {
            fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Số thứ tự không hợp lệ!\n");
            fmt::println("");
        }
        
    }
    catch (const std::exception&)
    {
        fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Lựa chọn không hợp lệ!\n");
        fmt::println("");
    }
}
