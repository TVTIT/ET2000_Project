#define FMT_HEADER_ONLY
#include <windows.h>
#include <iostream>
#include <string>
#include "rapidcsv.h"
#include "fmt/core.h"
#include "fmt/color.h"
#include <thread>
#include <fstream>
#include <sstream>
#include "ReadWriteCSV.h"
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
/// Hỗ trợ output bằng màu và đặt tiêu đề cho console
/// </summary>
void InitializeConsole()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hOut, dwMode);

	SetConsoleTitleW(L"Phần mềm điểm danh bằng thẻ sinh viên");
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

	hSerial = CreateFile(COM_Port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hSerial == INVALID_HANDLE_VALUE)
	{
		fmt::print(fmt::fg(fmt::color::black) | fmt::bg(fmt::color::yellow), "Không thể kết nối cổng COM. Hãy kiểm tra lại kết nối với thiết bị và nhập đúng cổng COM\n\n");
		fmt::println("Chuột phải vào This PC -> Manage -> Device Manager -> Ports (COM & LPT)");
		//fmt::print("Tìm tên Arduino UNO rồi nhập tên cổng vào (COMx): ");
		fmt::print("Tìm tên ");
		fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::gray), "Arduino UNO");
		fmt::print(" rồi nhập tên cổng vào (COMx): ");
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
		fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Không thể lấy trạng thái cổng COM. Hãy kiểm tra lại kết nối với thiết bị\n");
		PauseAndExit();
	}

	dcbSerialParams.BaudRate = CBR_9600;    // Tốc độ baud
	dcbSerialParams.ByteSize = 8;           // 8 bit dữ liệu
	dcbSerialParams.StopBits = ONESTOPBIT;  // 1 stop bit
	dcbSerialParams.Parity = NOPARITY;    // Không parity

	if (!SetCommState(hSerial, &dcbSerialParams))
	{
		fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Không thể cài đặt cấu hình cổng COM. Hãy kiểm tra lại kết nối với thiết bị\n");
		PauseAndExit();
	}

	// Cấu hình timeout
	timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(hSerial, &timeouts))
	{
		fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Không thể cài đặt timeout\n");
		PauseAndExit();
	}
}

void SentStartupCommand()
{
	char startupCommand[] = "startup";
	DWORD bytes_written;
	WriteFile(hSerial, startupCommand, sizeof(startupCommand), &bytes_written, NULL);
}

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
					fmt::print("Thẻ vừa quét trùng với thẻ của ");
					fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::gray), student_name);
					fmt::println(" đã có sẵn");
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
			fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Lỗi khi đọc dữ liệu từ cổng COM. Hãy kiểm tra lại kết nối với thiết bị\n");
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
					//fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), student_name + " đã điểm danh vào lúc " + GetTimeNow(0) + "\n");
					fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), "{0} đã điểm danh vào lúc {1}\n", student_name, GetTimeNow(0));
				}
				else
				{
					fmt::print(fmt::fg(fmt::color::black) | fmt::bg(fmt::color::yellow), "Không nhận dạng được thẻ sinh viên\n");
				}

			}
		}
		else
		{
			fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Lỗi khi đọc dữ liệu từ cổng COM. Hãy kiểm tra lại kết nối với thiết bị\n");
			fmt::println("Đang lưu kết quả điểm danh...\n\n");
			ReadWriteCSV::InKetQuaDiemDanh();
			ReadWriteCSV::LuuDuLieuDiemDanh();

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

/// <summary>
/// Đọc dữ liệu từ file text trong thẻ nhớ và thực hiện điểm danh
/// </summary>
void ReadTXTFileInSDCard()
{
	ClearScreen();
	fmt::println("Đang đọc dữ liệu từ thẻ nhớ...\n");

	char printCommand[] = "printTXTFile";
	DWORD bytes_written;
	if (WriteFile(hSerial, printCommand, sizeof(printCommand), &bytes_written, NULL))
	{
		char szBuff[2000];
		DWORD dwBytesRead = 0;
		string IDsCardFromTXT;

		if (ReadFile(hSerial, szBuff, sizeof(szBuff) - 1, &dwBytesRead, NULL))
		{
			if (dwBytesRead > 0) 
			{
				szBuff[dwBytesRead] = '\0';  // Kết thúc chuỗi
				IDsCardFromTXT = szBuff;
				
				if (IDsCardFromTXT == "sdcard_empty")
				{
					fmt::print(fmt::fg(fmt::color::black) | fmt::bg(fmt::color::yellow), "Không có dữ liệu trong thẻ nhớ\n");
				}
				else
				{
					stringstream stream_IDsCardFromTXT(IDsCardFromTXT);
					string segment;
					vector<string> IDsCardFromTXT_splited;

					while (getline(stream_IDsCardFromTXT, segment, '\n'))
					{
						IDsCardFromTXT_splited.push_back(segment);
					}

					for (int i = 0; i < IDsCardFromTXT_splited.size(); i++)
					{
						bool isIDCardExists = false;
						string student_name;

						ReadWriteCSV::GetStudentName(IDsCardFromTXT_splited[i], student_name, isIDCardExists);
						if (isIDCardExists)
						{
							fmt::println(student_name + " đã điểm danh");
						}
						else
						{
							fmt::println("ID thẻ {0} không nhận dạng được", IDsCardFromTXT_splited[i]);
						}
					}

					fmt::println("");
					fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), "Đọc dữ liệu hoàn tất! Nhấn phím Enter để tổng hợp kết quả...");
					fmt::println("");
					cin.get();

					ReadWriteCSV::InKetQuaDiemDanh();
					ReadWriteCSV::LuuDuLieuDiemDanh();
				}
			}
			else
			{
				fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Lỗi khi đọc dữ liệu từ cổng COM. Hãy kiểm tra lại kết nối với thiết bị\n");

				fmt::println("\nNhấn phím Enter để thử lại...");
				cin.get();
				ReadTXTFileInSDCard();
			}
		}

		
	}
	else
	{
		fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Lỗi khi gửi dữ liệu qua cổng COM. Hãy kiểm tra lại kết nối với thiết bị\n");

		fmt::println("\nNhấn phím Enter để thử lại...");
		cin.get();
		ReadTXTFileInSDCard();
	}
}

/// <summary>
/// Điểm danh không cần kết nối với máy tính
/// Cách thức hoạt động:
/// 1. Xoá dữ liệu điểm danh ở thẻ nhớ trước đó
/// 2. Cắm pin 9V vào arduino rồi quét thẻ như bình thường
/// 3. Sau khi hoàn tất, rút pin 9V rồi cắm arduino vào máy tính
/// 4. Gọi hàm ReadTXTFileInSDCard() để đọc dữ liệu từ file text qua Serial
/// </summary>
void DiemDanhKhongKetNoi()
{
	ClearScreen();
	fmt::println("Hãy chắc chắn thẻ nhớ đã được cắm vào thiết bị");
	fmt::print(fmt::fg(fmt::color::black) | fmt::bg(fmt::color::yellow), "CẢNH BÁO: Dữ liệu điểm danh được lưu trong thẻ nhớ trước đó sẽ bị xoá sạch\n");
	fmt::println("Nếu bạn muốn lưu lại kết quả điểm danh trước đó, hãy khởi động lại phần mềm");
	fmt::println("và chọn lựa chọn 5");

	fmt::println("Nhấn phím Enter để tiếp tục...");
	cin.get();

	char prepareCommand[] = "prepareForDisconnect";
	DWORD bytes_written;

	if (WriteFile(hSerial, prepareCommand, sizeof(prepareCommand), &bytes_written, NULL))
	{
		fmt::println("Sau khi rút thiết bị ra, hãy cắm nguồn 9V vào thiết bị");
		fmt::println("Sau đó thực hiện việc quét thẻ điểm danh như bình thường\n");

		fmt::println("Hãy rút thiết bị ra...\n");

		char szBuff[32] = { 0 };
		DWORD dwBytesRead = 0;
		while (ReadFile(hSerial, szBuff, sizeof(szBuff) - 1, &dwBytesRead, NULL))
		{

		}
		CloseHandle(hSerial);

		fmt::println("Khi thực hiện điểm danh xong, kết nối lại thiết bị");
		fmt::println("rồi nhấn Enter (hoặc chọn lựa chọn 5 ở màn hình chính)\n");

		fmt::println("Nhấn phím Enter để tiếp tục...");
		cin.get();

		InitializeRFID();
		ReadTXTFileInSDCard();
	}
	else
	{
		fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Lỗi khi gửi dữ liệu qua cổng COM. Hãy kiểm tra lại kết nối với thiết bị\n");

		fmt::println("\nNhấn phím Enter để thoát...");
		cin.get();

		exit(1);
	}
}

void MainInterface()
{
	ClearScreen();
	string user_input = "";
	fmt::println("[1] Thực hiện điểm danh");
	fmt::println("[2] Thêm thành viên vào lớp học");
	fmt::println("[3] Xoá thành viên khỏi lớp học");
	fmt::println("[4] Thực hiện điểm danh khi không kết nối máy tính");
	fmt::println("[5] Đọc dữ liệu trong thẻ nhớ và điểm danh");
	fmt::println("[6] Thoát");
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
		ReadWriteCSV::RemoveStudentFromCSV();
		PauseAndBack();
	}
	else if (user_input == "4")
	{
		DiemDanhKhongKetNoi();
		PauseAndBack();
	}
	else if (user_input == "5")
	{
		ReadTXTFileInSDCard();
		PauseAndBack();
	}
	else if (user_input == "6")
	{

	}
	else
	{
		fmt::print(fmt::fg(fmt::color::black) | fmt::bg(fmt::color::yellow), "Lựa chọn không hợp lệ!\n");
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
	InitializeConsole();

	ReadWriteCSV::GetPath();
	ReadWriteCSV::InitializeCSV();
	InitializeRFID();
	SentStartupCommand();
	
	MainInterface();

	CloseHandle(hSerial);
	return 0;
}
