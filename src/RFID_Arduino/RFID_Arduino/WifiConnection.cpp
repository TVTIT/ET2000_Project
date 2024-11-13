#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>
#include <vector>
#include <sstream>
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
bool WifiConnection::lastConnectionError = false;
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

    fd_set readfds;
    FD_ZERO(&readfds);

    timeval timeout;
    timeout.tv_sec = 3; //Nhớ chỉnh lại
    timeout.tv_usec = 0;

    fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), "Kết nối với thiết bị thành công!\n");
    fmt::println("\nĐưa thẻ sinh viên lại gần thiết bị quét...");
    fmt::println("Nhấn phím Enter để kết thúc quá trình điểm danh\n");
    while (isContinue)
    {
        FD_SET(sock, &readfds);
        ZeroMemory(buf, 1024);
        int selectResult = select(0, &readfds, NULL, NULL, &timeout);
        if (selectResult > 0)
        {
            lastConnectionError = false;
            int bytesRecevied = recv(sock, buf, 1024, 0);
            if (bytesRecevied > 0)
            {
                string str_input = string(buf, 0, bytesRecevied);

                stringstream stream_input(str_input);
                string segment;
                vector<string> input_splited;

                while (getline(stream_input, segment, '\n'))
                {
                    input_splited.push_back(segment);
                }

                string ID_Card = "";
                for (int i = 0; i < input_splited.size(); i++)
                {
                    if (input_splited[i] == "stillConnected" || input_splited[i].size() < 2)
                    {
                        continue;
                    }
                    ID_Card = input_splited[i];
                    break;
                }
                if (ID_Card.empty()) continue;
                
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
        else
        {
            fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::red), "Mất kết nối với thiết bị. Mã lỗi: {0}", WSAGetLastError());
            fmt::println("\nĐang kết nối lại...\n");
			closesocket(sock);
			WSACleanup();
            Reconnect(IP);
        }
    }

    isContinue = true;
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

    if (!lastConnectionError)
    {
        int sendResult = send(sock, "disconectWifi\n", 15, 0);
        Sleep(100);
    }
    
    isContinue = false;
    closesocket(sock);
    WSACleanup();
}

void WifiConnection::Reconnect(string IP)
{
    WSAData re_data;
    WORD re_ver = MAKEWORD(2, 2);
    WSAStartup(re_ver, &re_data);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in re_hint;
    re_hint.sin_family = AF_INET;
    re_hint.sin_port = htons(ESP8266_PORT);
    inet_pton(AF_INET, IP.c_str(), &re_hint.sin_addr);
    int re_connResult = connect(sock, (sockaddr*)&re_hint, sizeof(re_hint));

    if (re_connResult != SOCKET_ERROR)
    {
        fmt::print(fmt::fg(fmt::color::white) | fmt::bg(fmt::color::green), "Đã kết nối lại!");
        fmt::println("");
    }
}
