#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>
#include "fmt/core.h"
#include "fmt/base.h"
#include "fmt/chrono.h"
#include "fmt/color.h"

#include "WifiConnection.h"
#include "ReadWriteCSV.h"
#include "RFID_Arduino.h"

#define ESP8266_PORT 1234

using namespace std;

bool WifiConnection::isContinue = true;
SOCKET sock;

void WifiConnection::Connect(string IP) {
    //Khởi tạo Winsock
    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    if (wsResult != 0)
    {
        fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Lỗi khởi động winsock. Mã lỗi: {0}\n", wsResult);
        return;
    }

    //Tạo socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Lỗi tạo socket. Mã lỗi: {0}\n", WSAGetLastError());
        return;
    }

    //Thêm IP, port vào struct có sẵn
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(ESP8266_PORT);
    inet_pton(AF_INET, IP.c_str(), &hint.sin_addr);

    //Kết nối với server
    int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
    if (connResult == SOCKET_ERROR)
    {
        fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Kết nối với thiết bị không thành công. Mã lỗi: {0}\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return;
    }

    //Kết nối thành công
    char buf[1024];
    thread EnterHookThread(HookEnter);

    fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), "Kết nối với thiết bị thành công!\n");
    fmt::println("\nĐưa thẻ sinh viên lại gần thiết bị quét...");
    fmt::println("Nhấn phím Enter để kết thúc quá trình điểm danh\n");
    while (isContinue)
    {
        ZeroMemory(buf, 1024);
        int bytesRecevied = recv(sock, buf, 1024, 0);
        if (bytesRecevied > 0)
        {
            string ID_Card = string(buf, 0, bytesRecevied);
            string student_name;
            bool isIDCardExists;

            ReadWriteCSV::GetStudentName(ID_Card, student_name, isIDCardExists);
            if (isIDCardExists)
            {
                fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), "{0} đã điểm danh vào lúc {1}", student_name, GetTimeNow(0));
                fmt::println("");
            }
            else
            {
                fmt::print(fmt::fg(fmt::color::black) | fmt::bg(fmt::color::yellow), "Không nhận dạng được thẻ sinh viên");
                fmt::println("");
            }
        }
    }

    ClearScreen();
    ReadWriteCSV::InKetQuaDiemDanh();
    ReadWriteCSV::LuuDuLieuDiemDanh();

    PauseAndBack();

    EnterHookThread.join();

}

void WifiConnection::HookEnter()
{
    string temp;
    getline(cin, temp);
    closesocket(sock);
    WSACleanup();
    isContinue = false;
}
