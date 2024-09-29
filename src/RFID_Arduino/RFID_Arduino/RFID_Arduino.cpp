#define FMT_HEADER_ONLY
#include <windows.h>
#include <iostream>
#include <string>
#include "rapidcsv.h"
#include <fmt/core.h>
#include <thread>
#include <fstream>
#include "ReadWriteCSV.h"
//#include "ReadWriteCSV.cpp"

using namespace std;

void MainInterface();
void PauseAndExit();
void PauseAndBack();

bool isContinue = true;
wstring COM_Port;

HANDLE hSerial;
DCB dcbSerialParams;
COMMTIMEOUTS timeouts;

/// <summary>
/// Clear hết chữ trên console
/// </summary>
void ClearScreen()
{
	HANDLE                     hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD                      count;
	DWORD                      cellCount;
	COORD                      homeCoords = { 0, 0 };

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) return;

	/* Get the number of cells in the current buffer */
	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi)) return;
	cellCount = csbi.dwSize.X * csbi.dwSize.Y;

	/* Fill the entire buffer with spaces */
	if (!FillConsoleOutputCharacter(
		hStdOut,
		(TCHAR)' ',
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Fill the entire buffer with the current colors and attributes */
	if (!FillConsoleOutputAttribute(
		hStdOut,
		csbi.wAttributes,
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Move the cursor home */
	SetConsoleCursorPosition(hStdOut, homeCoords);
}

/// <summary>
/// Dùng cho thread để hook được nút enter, từ đó kết thúc quá trình điểm danh
/// </summary>
void HookEnter()
{
	string temp;
	getline(cin, temp);
	//fmt::println("Điểm danh hoàn tất! Đang lưu dữ liệu điểm danh...");
	isContinue = false;
}

/// <summary>
/// Khởi tạo kết nối với module
/// </summary>
void InitializeRFID()
{
	// Mở cổng COM (kiểm tra COM port của Arduino trong Device Manager, ví dụ COM3)
	wfstream COM_Port_File;
	COM_Port_File.open(ReadWriteCSV::DirectoryPath + "\\COM_Port.dat");

	if (COM_Port_File)
	{
		getline(COM_Port_File, COM_Port);
	}
	/*else
	{
		fmt::println("Chuột phải vào This PC -> Manage -> Device Manager -> Ports (COM & LPT)");
		fmt::print("Tìm tên Arduino UNO rồi nhập tên cổng vào (COMx): ");
		getline(wcin, COM_Port);
		COM_Port_File.open(ReadWriteCSV::DirectoryPath + "\\COM_Port.dat", std::ofstream::out | std::ofstream::app);
		COM_Port_File << COM_Port;
		COM_Port_File.close();
	}*/

	hSerial = CreateFile(COM_Port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hSerial == INVALID_HANDLE_VALUE)
	{
		fmt::println("Không thể kết nối cổng COM. Hãy kiểm tra lại kết nối với thiết bị và nhập đúng cổng COM\n");
		fmt::println("Chuột phải vào This PC -> Manage -> Device Manager -> Ports (COM & LPT)");
		fmt::print("Tìm tên Arduino UNO rồi nhập tên cổng vào (COMx): ");
		getline(wcin, COM_Port);
		COM_Port_File.close();
		COM_Port_File.open(ReadWriteCSV::DirectoryPath + "\\COM_Port.dat", std::ofstream::out | std::ofstream::trunc);
		COM_Port_File << COM_Port;
		COM_Port_File.close();

		InitializeRFID();
	}

	// Cài đặt cấu hình serial
	dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (!GetCommState(hSerial, &dcbSerialParams))
	{
		fmt::println("Không thể lấy trạng thái cổng COM. Hãy kiểm tra lại kết nối với thiết bị");
		PauseAndExit();
	}

	dcbSerialParams.BaudRate = CBR_9600;    // Tốc độ baud
	dcbSerialParams.ByteSize = 8;           // 8 bit dữ liệu
	dcbSerialParams.StopBits = ONESTOPBIT;  // 1 stop bit
	dcbSerialParams.Parity = NOPARITY;    // Không parity

	if (!SetCommState(hSerial, &dcbSerialParams))
	{
		fmt::println("Không thể cài đặt cấu hình cổng COM. Hãy kiểm tra lại kết nối với thiết bị");
		PauseAndExit();
	}

	// Cấu hình timeout
	timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(hSerial, &timeouts))
	{
		fmt::println("Không thể cài đặt timeout");
		PauseAndExit();
	}
}

//void InitializeRFID()
//{
//	bool isCOMAvailable = false;
//
//	//// Cài đặt cấu hình serial
//	//dcbSerialParams = { 0 };
//	//dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
//
//	//dcbSerialParams.BaudRate = CBR_9600;    // Tốc độ baud
//	//dcbSerialParams.ByteSize = 8;           // 8 bit dữ liệu
//	//dcbSerialParams.StopBits = ONESTOPBIT;  // 1 stop bit
//	//dcbSerialParams.Parity = NOPARITY;    // Không parity
//
//	//// Cấu hình timeout
//	//timeouts = { 0 };
//	//timeouts.ReadIntervalTimeout = 50;
//	//timeouts.ReadTotalTimeoutConstant = 50;
//	//timeouts.ReadTotalTimeoutMultiplier = 10;
//
//	for (int i = 1; i <= 10; i++)
//	{
//		wstring wstr_COM_Port = L"COM" + to_wstring(i);
//		const wchar_t* wchar_COM_Port = wstr_COM_Port.c_str();
//		hSerial = CreateFile(wchar_COM_Port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
//
//		if (hSerial != INVALID_HANDLE_VALUE)
//		{
//			isCOMAvailable = true;
//			break;
//		}
//	}
//
//	if (!isCOMAvailable)
//	{
//		fmt::println("Không thể mở cổng COM. Hãy kiểm tra lại kết nối với thiết bị");
//		PauseAndExit();
//	}
//
//	// Cài đặt cấu hình serial
//	dcbSerialParams = { 0 };
//	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
//	if (!GetCommState(hSerial, &dcbSerialParams))
//	{
//		fmt::println("Không thể lấy trạng thái cổng COM. Hãy kiểm tra lại kết nối với thiết bị");
//		PauseAndExit();
//	}
//
//	dcbSerialParams.BaudRate = CBR_9600;    // Tốc độ baud
//	dcbSerialParams.ByteSize = 8;           // 8 bit dữ liệu
//	dcbSerialParams.StopBits = ONESTOPBIT;  // 1 stop bit
//	dcbSerialParams.Parity = NOPARITY;    // Không parity
//
//	if (!SetCommState(hSerial, &dcbSerialParams))
//	{
//		fmt::println("Không thể cài đặt cấu hình cổng COM. Hãy kiểm tra lại kết nối với thiết bị");
//		PauseAndExit();
//	}
//
//	// Cấu hình timeout
//	timeouts = { 0 };
//	timeouts.ReadIntervalTimeout = 50;
//	timeouts.ReadTotalTimeoutConstant = 50;
//	timeouts.ReadTotalTimeoutMultiplier = 10;
//
//	if (!SetCommTimeouts(hSerial, &timeouts))
//	{
//		fmt::println("Không thể cài đặt timeout");
//		PauseAndExit();
//	}
//}

/// <summary>
/// Hàm để lấy thời gian hiện tại theo hệ thông, trả về string
/// </summary>
/// <param name="type">Loại format dùng, 0 là để print còn lại để đặt tên file</param>
/// <returns></returns>
string GetTimeNow(int type)
{
	auto now = std::chrono::system_clock::now();
	std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

	// Chuyển đổi sang cấu trúc tm
	std::tm local_time;
	localtime_s(&local_time, &now_time_t);

	std::ostringstream oss;
	type == 0 ? oss << std::put_time(&local_time, "%H:%M:%S %d/%m/%Y") : oss << std::put_time(&local_time, "%d-%m-%Y %H-%M-%S");

	return oss.str();
}

void AddStudent()
{
	ClearScreen();
	fmt::println("Đưa thẻ sinh viên lại gần thiết bị quét...");

	isContinue = true;
	// Vòng lặp để đọc dữ liệu từ Arduino
	char szBuff[32] = { 0 };
	DWORD dwBytesRead = 0;
	while (true)
	{
		if (ReadFile(hSerial, szBuff, sizeof(szBuff) - 1, &dwBytesRead, NULL))
		{
			if (dwBytesRead > 0)
			{
				szBuff[dwBytesRead] = '\0';  // Kết thúc chuỗi
				string str_szBuff = szBuff;
				bool isIDCardExists = false;
				string student_name;

				ReadWriteCSV::GetStudentName(str_szBuff, student_name, isIDCardExists);
				if (isIDCardExists)
				{
					fmt::println("Thẻ vừa quét trùng với thẻ của " + student_name + " đã có sẵn");
					break;
				}
				else
				{
					ReadWriteCSV::AddStudentToCSV(str_szBuff);
					break;
				}
			}
		}
		else
		{
			fmt::println("Lỗi khi đọc dữ liệu từ cổng COM. Hãy kiểm tra lại kết nối với thiết bị");
			PauseAndExit();
		}
	}
	PauseAndBack();
}

void DiemDanh()
{
	ClearScreen();
	fmt::println("Đưa thẻ sinh viên lại gần thiết bị quét...");
	fmt::println("Nhấn phím Enter để kết thúc quá trình điểm danh\n");

	isContinue = true;
	//Tạo thread để hook nút enter để ngừng quá trình điểm danh
	thread EnterHookThread(HookEnter);

	// Vòng lặp để đọc dữ liệu từ Arduino
	char szBuff[32] = { 0 };
	DWORD dwBytesRead = 0;
	while (isContinue)
	{
		if (ReadFile(hSerial, szBuff, sizeof(szBuff) - 1, &dwBytesRead, NULL))
		{
			if (dwBytesRead > 0)
			{
				szBuff[dwBytesRead] = '\0';  // Kết thúc chuỗi
				string str_szBuff = szBuff;
				bool isIDCardExists = false;
				string student_name;

				ReadWriteCSV::GetStudentName(str_szBuff, student_name, isIDCardExists);
				if (isIDCardExists)
				{
					fmt::println(student_name + " đã điểm danh vào lúc " + GetTimeNow(0));
				}
				else
				{
					fmt::println("Không nhận dạng được thẻ sinh viên");
				}

			}
		}
		else
		{
			fmt::println("Lỗi khi đọc dữ liệu từ cổng COM. Hãy kiểm tra lại kết nối với thiết bị");
			fmt::println("Đang lưu kết quả điểm danh...\n\n");
			//EnterHookThread.detach();
			//std::abort();
			//bool abc = TerminateThread(EnterHookThread.native_handle(), 1);
			ReadWriteCSV::InKetQuaDiemDanh();
			ReadWriteCSV::LuuDuLieuDiemDanh();
			//PauseAndExit();

			//Do còn EnterHookThread đang chạy nên gọi hàm PauseAndExit() sẽ phải bấm enter 2 lần mới thoát được
			fmt::println("\nNhấn phím Enter để thoát...");
			EnterHookThread.join();
			exit(1);
		}
	}

	ClearScreen();
	ReadWriteCSV::InKetQuaDiemDanh();
	ReadWriteCSV::LuuDuLieuDiemDanh();

	PauseAndBack();
	EnterHookThread.join();
}

void MainInterface()
{
	ClearScreen();
	string user_input = "";
	fmt::println("[1] Thực hiện điểm danh");
	fmt::println("[2] Thêm thành viên vào lớp học");
	fmt::println("[3] Thoát");
	fmt::print("Nhập lựa chọn của bạn: ");

	getline(cin, user_input);
	if (user_input == "1")
	{
		DiemDanh();
	}
	else if (user_input == "2")
	{
		AddStudent();
	}
	else if (user_input == "3")
	{

	}
	else
	{
		fmt::println("Lựa chọn không hợp lệ!");
		PauseAndBack();
	}
}

void PauseAndExit()
{
	fmt::println("Nhấn phím enter để thoát...");
	cin.get();
	CloseHandle(hSerial);
	exit(1);
}

void PauseAndBack()
{
	fmt::println("Nhấn phím enter để quay lại...");
	cin.get();
	MainInterface();
}

int main()
{
	ReadWriteCSV::GetPath();
	ReadWriteCSV::InitializeCSV();
	InitializeRFID();

	//Đặt tiêu đề cho console (hiện trên đầu cửa sổ)
	SetConsoleTitleW(L"Phần mềm điểm danh bằng thẻ sinh viên");

	MainInterface();

	CloseHandle(hSerial);
	return 0;
}
