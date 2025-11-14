//Service.cpp

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <mutex>
#include "DbConnect.h"
#pragma comment(lib, "Ws2_32.lib")
#include <windows.h>

#include "RegLogin.h"
//#include <string>
#include <sstream>
#include "DebugLog.h"

#define SERVICE_NAME L"ChatAppService" // Service name

// Các biến toàn cục cho service
SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE; //stop service
SOCKET                g_listenSocket = INVALID_SOCKET;
HANDLE                g_hWorkerThread = NULL; // handle workerthread

// Khai báo các hàm
VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
DWORD WINAPI ClientThreadHandler(LPVOID p);

std::vector<SOCKET> g_clientSockets; // Danh sách socket của client
std::mutex g_socketMutex;


// Khởi tạo WMain
int wmain(int argc, wchar_t* argv[]);
int WINAPI WinMain(
    _In_ HINSTANCE hInst,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    return wmain(__argc, __wargv);
}


// Hàm wmain, khai báo service
int wmain(int argc, wchar_t* argv[])
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {(LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        return GetLastError();
    }

    return 0;
}


bool SendResponse(SOCKET clientSocket, const std::string& response)
{
    std::string msg = response + "\n";
    int sendResult = send(clientSocket, msg.c_str(), (int)msg.length(), 0);
    if (sendResult == SOCKET_ERROR) {
        //DEBUG_LOG(L"Lỗi send: %d", WSAGetLastError());
        return false;
    }
    return true;
}


std::string ResponseToString(AuthResponse response)
{
    switch (response) {
    case AuthResponse::SUCCESS_LOGIN:       return "LOGIN_OK";
    case AuthResponse::SUCCESS_REGISTER:    return "REG_OK";
    case AuthResponse::FAILED_USER_EXISTS:  return "ERR_USER_EXISTS";
    case AuthResponse::FAILED_NOT_FOUND:    return "ERR_NOT_FOUND";
    case AuthResponse::FAILED_WRONG_PASS:   return "ERR_WRONG_PASS";
    case AuthResponse::FAILED_DB_ERROR:     return "ERR_DB_ERROR";
    case AuthResponse::FAILED_INVALID_INPUT:return "ERR_INVALID_INPUT";
    default:                                return "ERR_UNKNOWN";
    }
}



// Thread Handler
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    // Khởi tạo Winsock, tạo socket, bind và listen
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { 
        //DEBUG_LOG(L"WSAStartup thất bại ", WSAGetLastError());
        return 1; 
    }

    //connect db
    if (!ConnectDatabase())
    {
        //DEBUG_LOG(L"Không thể kết nối Database, service dừng.");
        WSACleanup();
        return 1;
    }

    g_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // tcp
    if (g_listenSocket == INVALID_SOCKET) { return 1; }

    // Config server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9999);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // error
    if (bind(g_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) { 
        //DEBUG_LOG(L"Bind socket thất bại ", WSAGetLastError()); 
        return 1;
    }
    if (listen(g_listenSocket, SOMAXCONN) == SOCKET_ERROR) { return 1; }


    std::vector<HANDLE> clientThreads;

    // Vòng lặp chính của service, chờ client và chờ tín hiệu dừng
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        SOCKET clientSocket = accept(g_listenSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            // Thêm socket vào danh sách được bảo vệ
            {
                std::lock_guard<std::mutex> lock(g_socketMutex);
                g_clientSockets.push_back(clientSocket);
            }
            // tạo luồng cho client mới
            HANDLE h = CreateThread(
                nullptr, 0,
                ClientThreadHandler,
                (LPVOID)(uintptr_t)clientSocket,
                0, nullptr);

            if (h) {
                clientThreads.push_back(h);
            }
            else {
                // Tạo luồng thất bại → đóng socket client
                closesocket(clientSocket);
            }
        }
        else {
            break;
        }
    }

    // Đóng tất cả các socket client đang hoạt động để unblock các luồng đang chờ recv()
    {
        std::lock_guard<std::mutex> lock(g_socketMutex);
        for (SOCKET clientSocket : g_clientSockets) {
            shutdown(clientSocket, SD_BOTH);
            closesocket(clientSocket);
        }
        g_clientSockets.clear(); // Xóa sạch danh sách
    }

    // Chờ client kết thúc
    const DWORD MAX_WAIT = 64;
    for (size_t i = 0; i < clientThreads.size(); i += MAX_WAIT) {
        DWORD n = (DWORD)(((clientThreads.size() - i) < (size_t)MAX_WAIT)
            ? (clientThreads.size() - i)
            : (size_t)MAX_WAIT);
        WaitForMultipleObjects(n, &clientThreads[i], TRUE, INFINITE);
        for (DWORD k = 0; k < n; ++k) {
            CloseHandle(clientThreads[i + k]);
        }
    }
    clientThreads.clear();

    DisconnectDatabase();
    WSACleanup();
    return 0;
}

// quản lý trạng thái service
VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    // Đăng ký hàm xử lý các lệnh điều khiển
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    if (g_StatusHandle == NULL) {
        //DEBUG_LOG(L"RegisterServiceCtrlHandler thất bại ", GetLastError());
    return; }

    // Báo trạng thái ban đầu
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 1;
    g_ServiceStatus.dwWaitHint = 3000;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // Event để báo hiệu dừng service
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL) {
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        //DEBUG_LOG(L"CreateEvent thất bại ", GetLastError());
        return;
    }

    // BÁO CÁO SERVICE ĐÃ CHẠY THÀNH CÔNG
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP; // chỉ stop
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // Start workerThread để xử lý logic chính
    g_hWorkerThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    if (g_hWorkerThread == NULL) { //error
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        //DEBUG_LOG(L"CreateThread thất bại ", GetLastError());
        return;
    }

    // Chờ worker thread kết thúc
    WaitForSingleObject(g_hWorkerThread, INFINITE);

    // Dọn dẹp trước khi thoát ServiceMain
    CloseHandle(g_ServiceStopEvent);
    CloseHandle(g_hWorkerThread);

    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

// xử lý stop clean
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING)
        {
            // Báo cho SCM biết đang dừng
            g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            g_ServiceStatus.dwCheckPoint++;
            g_ServiceStatus.dwWaitHint = 5000;
            SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

            // Gửi tín hiệu dừng cho các vòng lặp
            SetEvent(g_ServiceStopEvent);

            // Đóng socket lắng nghe để `accept` thoát ra ngay lập tức
            if (g_listenSocket != INVALID_SOCKET) {
                shutdown(g_listenSocket, SD_BOTH);
                closesocket(g_listenSocket);
                g_listenSocket = INVALID_SOCKET;
            }
        }
        break;
    default:
        break;
    }
}


// Hàm chờ client và xử lý xác thực/chat (ĐÃ CẬP NHẬT)
DWORD WINAPI ClientThreadHandler(LPVOID p) {
    SOCKET clientSocket = (SOCKET)(uintptr_t)p;
    char buffer[1024];
    int recvResult;
    bool loggedIn = false;
    std::string loggedInUsername = ""; // Lưu lại username khi đăng nhập thành công

    //DEBUG_LOG(L"Client [%d] kết nối. Chờ xác thực...", (int)clientSocket);

    while (!loggedIn)
    {
        recvResult = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (recvResult <= 0) {
            // Client đã ngắt kết nối trước khi đăng nhập thành công
            //DEBUG_LOG(L"Client [%d] ngắt kết nối trong khi xác thực.", (int)clientSocket);
            break; // Thoát khỏi vòng lặp while(!loggedIn)
        }

        buffer[recvResult] = '\0';
        std::string commandLine(buffer);

        // Xóa các ký tự \r \n ở cuối (nếu client gửi)
        while (!commandLine.empty() && (commandLine.back() == '\r' || commandLine.back() == '\n')) {
            commandLine.pop_back();
        }

        std::stringstream ss(commandLine);
        std::string command, username, password;

        ss >> command >> username >> password;

        AuthResponse response = AuthResponse::FAILED_INVALID_INPUT;

        if (command == "REG" && !username.empty() && !password.empty())
        {
            //DEBUG_LOG(L"Client [%d] yêu cầu REG user: %S", (int)clientSocket, username.c_str());
            response = ProcessRegistration(username, password);
        }
        else if (command == "LOGIN" && !username.empty() && !password.empty())
        {
            //DEBUG_LOG(L"Client [%d] yêu cầu LOGIN user: %S", (int)clientSocket, username.c_str());
            response = ProcessLogin(username, password);

            if (response == AuthResponse::SUCCESS_LOGIN) {
                loggedIn = true;
                loggedInUsername = username; // Lưu lại tên user
            }
        }
        else
        {
            //DEBUG_LOG(L"Client [%d] gửi lệnh không hợp lệ: %S", (int)clientSocket, commandLine.c_str());
        }

        SendResponse(clientSocket, ResponseToString(response));

        if (loggedIn) {
            break;
        }

    }



    if (loggedIn)
    {
        //DEBUG_LOG(L"Client [%d] (User: %S) đã vào vòng lặp chat.", (int)clientSocket, loggedInUsername.c_str());

        do {
            recvResult = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (recvResult > 0)
            {
                // Client gửi tin nhắn
                buffer[recvResult] = '\0';
                std::string message(buffer);
                //DEBUG_LOG(L"Client [%d] (User: %S) gửi: %S", (int)clientSocket, loggedInUsername.c_str(), message.c_str());

            }
            else
            {
                // Client ngắt kết nối (recvResult <= 0)
                break;
            }

        } while (true);

        //DEBUG_LOG(L"Client [%d] (User: %S) đã ngắt kết nối (sau khi đăng nhập).", (int)clientSocket, loggedInUsername.c_str());
    }
    else
    {
        //DEBUG_LOG(L"Client [%d] xác thực thất bại và đã ngắt kết nối, đóng luồng.", (int)clientSocket);
    }


    shutdown(clientSocket, SD_SEND);
    closesocket(clientSocket);

    // Xóa khỏi danh sách
    {
        std::lock_guard<std::mutex> lock(g_socketMutex);
        for (auto it = g_clientSockets.begin(); it != g_clientSockets.end(); ++it) {
            if (*it == clientSocket) {
                g_clientSockets.erase(it);
                break;
            }
        }
    }

    //DEBUG_LOG(L"Đã dọn dẹp và kết thúc luồng cho Client [%d]", (int)clientSocket);
    return 0;
}

