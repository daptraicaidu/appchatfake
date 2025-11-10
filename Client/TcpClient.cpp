// TcpClient.cpp

#include "pch.h" 
#include "TcpClient.h"

TcpClient::TcpClient()
    : m_socket(INVALID_SOCKET),
    m_isConnected(false),
    m_wsaInitialized(false)
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        // Không thể khởi tạo Winsock, đây là lỗi nghiêm trọng
        // Bạn có thể ném ngoại lệ hoặc xử lý cách khác
        throw std::runtime_error("WSAStartup failed!");
    }
    m_wsaInitialized = true;
}

TcpClient::~TcpClient()
{
    Disconnect(); // Tự động ngắt kết nối khi đối tượng bị hủy
    if (m_wsaInitialized) {
        WSACleanup();
    }
}

bool TcpClient::Connect(const char* ip, int port)
{
    if (m_isConnected) {
        return true; // Đã kết nối rồI
    }

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        return false; // Lỗi tạo socket
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    if (connect(m_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false; // Lỗi kết nối
    }

    m_isConnected = true;
    return true;
}

void TcpClient::Disconnect()
{
    if (m_socket != INVALID_SOCKET) {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    m_isConnected = false;
}

bool TcpClient::SendRequest(const std::string& request)
{
    if (!m_isConnected) {
        return false;
    }
    // Gửi dữ liệu, thêm ký tự '\n' để service biết kết thúc lệnh
    std::string msg = request + "\n";
    int sendResult = send(m_socket, msg.c_str(), (int)msg.length(), 0);

    return (sendResult != SOCKET_ERROR);
}

std::string TcpClient::ReceiveResponse()
{
    if (!m_isConnected) {
        return "ERR_NOT_CONNECTED";
    }

    char buffer[1024] = { 0 };
    // Hàm này sẽ đứng chờ (blocking) cho đến khi nhận được dữ liệu
    int recvResult = recv(m_socket, buffer, sizeof(buffer) - 1, 0);

    if (recvResult <= 0) {
        // Lỗi hoặc server ngắt kết nối
        Disconnect();
        return "ERR_CONNECTION_LOST";
    }

    buffer[recvResult] = '\0';

    // Xóa ký tự \n ở cuối nếu có
    std::string response(buffer);
    while (!response.empty() && (response.back() == '\r' || response.back() == '\n')) {
        response.pop_back();
    }

    return response;
}

void TcpClient::DetachSocket()
{
    // Khi gọi hàm này, đối tượng TcpClient sẽ "bỏ" socket lại
    // để nó không bị đóng khi TcpClient bị hủy (ví dụ: khi dialog login tắt)
    m_socket = INVALID_SOCKET;
    m_isConnected = false;
}