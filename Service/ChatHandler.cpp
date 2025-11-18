// ChatHandler.cpp

#include "ChatHandler.h"
#include "DebugLog.h"
#include "DbConnect.h"
#include <sstream>
#include <map>
#include <mutex>
#include <vector>

// Khai báo các biến map online từ Service.cpp

extern std::map<std::string, SOCKET> g_onlineUsers;
extern std::mutex g_onlineUsersMutex;

// Hàm SendResponse cũng từ Service.cpp
bool SendResponse(SOCKET clientSocket, const std::string& response);


// Lấy UserId từ Username.
int GetUserId(const std::string& username)
{
    if (!g_db) return -1;

    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT UserId FROM Users WHERE Username = ?;";
    int userId = -1;

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        DEBUG_LOG(L"Lỗi prepare (GetUserId): %S", sqlite3_errmsg(g_db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return userId;
}

// Save Message to DB
sqlite3_int64 SaveMessageToDb(int senderId, int receiverId, const std::string& content)
{
    if (!g_db) return -1;

    sqlite3_stmt* stmt = NULL;
    const char* sql = "INSERT INTO Messages (SenderId, ReceiverId, Content, SentDate) "
        "VALUES (?, ?, ?, datetime('now', 'localtime'));";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        DEBUG_LOG(L"Lỗi prepare (SaveMessage): %S", sqlite3_errmsg(g_db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, senderId);
    sqlite3_bind_int(stmt, 2, receiverId);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        DEBUG_LOG(L"Lỗi step (SaveMessage): %S", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_int64 lastId = sqlite3_last_insert_rowid(g_db);
    sqlite3_finalize(stmt);

    DEBUG_LOG(L"Đã lưu tin nhắn (ID: %lld) từ %d đến %d", lastId, senderId, receiverId);
    return lastId;
}



// Xử lý lệnh từ client.

void ProcessClientMessage(SOCKET senderSocket, const std::string& senderUsername, const std::string& messageLine)
{
    std::stringstream ss(messageLine);
    std::string command;
    ss >> command;

    // Lệnh SEND
    if (command == "SEND")
    {
        // --- 1. Phân tích lệnh ---
        // Định dạng: SEND <recipient_username> <Nội dung tin nhắn>
        std::string recipientUsername;
        std::string messageContent;

        ss >> recipientUsername;

        // Lấy phần còn lại của dòng làm nội dung tin nhắn
        std::getline(ss, messageContent);
        // Xóa khoảng trắng thừa ở đầu
        if (!messageContent.empty() && messageContent[0] == ' ') {
            messageContent.erase(0, 1);
        }

        if (recipientUsername.empty() || messageContent.empty()) {
            DEBUG_LOG(L"Lệnh SEND không hợp lệ từ %S", senderUsername.c_str());
            SendResponse(senderSocket, "ERR_INVALID_MSG_FORMAT");
            return;
        }

        // --- 2. Lấy ID từ DB ---
        int senderId = GetUserId(senderUsername);
        int recipientId = GetUserId(recipientUsername);

        if (senderId == -1) {
            DEBUG_LOG(L"Không tìm thấy SenderId cho %S", senderUsername.c_str());
            SendResponse(senderSocket, "ERR_FATAL_SENDER"); // Lỗi nghiêm trọng
            return;
        }
        if (recipientId == -1) {
            DEBUG_LOG(L"Không tìm thấy RecipientId cho %S", recipientUsername.c_str());
            SendResponse(senderSocket, "ERR_RECIPIENT_NOT_FOUND");
            return;
        }

        // --- 3. Lưu tin nhắn vào DB ---
        sqlite3_int64 messageId = SaveMessageToDb(senderId, recipientId, messageContent);
        if (messageId == -1) {
            SendResponse(senderSocket, "ERR_SAVE_MSG_FAILED");
            return;
        }

        // Phản hồi cho người gửi là "OK"
        SendResponse(senderSocket, "SEND_OK");

        // --- 4. Kiểm tra người nhận online và chuyển tiếp ---
        SOCKET recipientSocket = INVALID_SOCKET;

        // Khóa mutex để truy cập map g_onlineUsers
        {
            std::lock_guard<std::mutex> lock(g_onlineUsersMutex);
            auto it = g_onlineUsers.find(recipientUsername);
            if (it != g_onlineUsers.end()) {
                // Tìm thấy! Người nhận đang online
                recipientSocket = it->second;
            }
        } // Mutex được tự động mở khóa ở đây

        if (recipientSocket != INVALID_SOCKET)
        {
            // Người nhận đang online, gửi tin nhắn cho họ
            // Định dạng: RECV <sender_username> <Nội dung tin nhắn>
            std::string forwardMessage = "RECV " + senderUsername + " " + messageContent;

            if (SendResponse(recipientSocket, forwardMessage))
            {
                // Gửi thành công, cập nhật DB
                DEBUG_LOG(L"Đã chuyển tiếp tin nhắn %lld đến %S (Online)", messageId, recipientUsername.c_str());
            }
            else
            {
                // Bị lỗi khi gửi (có thể client vừa ngắt kết nối)
                DEBUG_LOG(L"Lỗi khi chuyển tiếp tin nhắn %lld đến %S (Socket Error)", messageId, recipientUsername.c_str());
            }
        }
        else
        {
            // Người nhận offline, tin nhắn đã được lưu (IsDelivered = 0)
            DEBUG_LOG(L"Người nhận %S đang offline, tin nhắn %lld sẽ được gửi sau.", recipientUsername.c_str(), messageId);
        }
    }
    else if (command == "GET_USERS")
    {
        HandleGetUsersList(senderSocket, senderUsername);
    }
    else if (command == "GET_HISTORY")
    {
        std::string otherUsername;
        ss >> otherUsername;
        HandleGetHistory(senderSocket, senderUsername, otherUsername);
    }
    // TODO: Thêm các lệnh khác
    else
    {
        DEBUG_LOG(L"Lệnh không xác định '%S' từ %S", command.c_str(), senderUsername.c_str());
        SendResponse(senderSocket, "ERR_UNKNOWN_COMMAND");
    }
}


// GET userlist
void HandleGetUsersList(SOCKET senderSocket, const std::string& currentUsername)
{
    if (!g_db) {
        SendResponse(senderSocket, "ERR_DB_ERROR");
        return;
    }

    sqlite3_stmt* stmt = NULL;
    // Lấy tất cả Username, sắp xếp theo tên
    const char* sql =
        "SELECT Username FROM Users "
        "WHERE Username <> ? "
        "ORDER BY Username;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        DEBUG_LOG(L"Lỗi prepare (GetUsersList): %S", sqlite3_errmsg(g_db));
        SendResponse(senderSocket, "ERR_DB_ERROR");
        return;
    }

    // Bind username hiện tại để loại ra
    sqlite3_bind_text(
        stmt,
        1,
        currentUsername.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    // Định dạng phản hồi: USERS_LIST <user1> <user2> <user3> ...
    // Ví dụ: USERS_LIST an admin testuser
    std::string response = "USERS_LIST";

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const char* username = (const char*)sqlite3_column_text(stmt, 0);
        response += " ";
        response += username;
    }

    sqlite3_finalize(stmt);

    // Gửi danh sách về cho client
    SendResponse(senderSocket, response);
    DEBUG_LOG(L"Đã gửi danh sách user cho Client [%d]", (int)senderSocket);
}



// Lấy lịch sự chat
void HandleGetHistory(SOCKET senderSocket, const std::string& senderUsername, const std::string& otherUsername)
{
    if (otherUsername.empty()) {
        SendResponse(senderSocket, "ERR_INVALID_HISTORY_REQUEST");
        return;
    }

    // --- 1. Lấy ID của cả 2 user ---
    int senderId = GetUserId(senderUsername);
    int otherId = GetUserId(otherUsername);

    if (senderId == -1 || otherId == -1) {
        DEBUG_LOG(L"Không tìm thấy ID cho %S hoặc %S", senderUsername.c_str(), otherUsername.c_str());
        SendResponse(senderSocket, "ERR_USER_NOT_FOUND");
        return;
    }

    // --- 2. Chuẩn bị câu SQL ---
    if (!g_db) {
        SendResponse(senderSocket, "ERR_DB_ERROR");
        return;
    }

    sqlite3_stmt* stmt = NULL;
    // Lấy các tin nhắn giữa 2 người (senderId, otherId)
    // Sắp xếp theo ngày gửi
    const char* sql = "SELECT SenderId, Content, SentDate FROM Messages "
        "WHERE (SenderId = ? AND ReceiverId = ?) OR (SenderId = ? AND ReceiverId = ?) "
        "ORDER BY SentDate ASC;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        DEBUG_LOG(L"Lỗi prepare (GetHistory): %S", sqlite3_errmsg(g_db));
        SendResponse(senderSocket, "ERR_DB_ERROR");
        return;
    }

    // Bind các ID
    sqlite3_bind_int(stmt, 1, senderId);
    sqlite3_bind_int(stmt, 2, otherId);
    sqlite3_bind_int(stmt, 3, otherId);
    sqlite3_bind_int(stmt, 4, senderId);

    // --- 3. Gửi phản hồi (có thể nhiều tin) ---
    // Gửi tin nhắn bắt đầu
    // Định dạng: HISTORY_START <other_username>
    SendResponse(senderSocket, "HISTORY_START " + otherUsername);
    DEBUG_LOG(L"Bắt đầu gửi lịch sử chat giữa %S và %S", senderUsername.c_str(), otherUsername.c_str());

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int msgSenderId = sqlite3_column_int(stmt, 0);
        const char* content = (const char*)sqlite3_column_text(stmt, 1);
        // const char* sentDate = (const char*)sqlite3_column_text(stmt, 2); // Tạm thời chưa dùng ngày

        // Xác định tin nhắn này là "của tôi" (MSG_ME) hay "của họ" (MSG_THEM)
        std::string msgType = (msgSenderId == senderId) ? "MSG_ME" : "MSG_THEM";

        // Định dạng: <MSG_TYPE> <Nội dung>
        // Ví dụ: MSG_ME Hôm nay ăn gì?
        // Ví dụ: MSG_THEM Ăn phở.
        std::string historyLine = msgType + " " + std::string(content);

        // Gửi từng dòng tin nhắn
        SendResponse(senderSocket, historyLine);
    }

    sqlite3_finalize(stmt);

    // Gửi tin nhắn kết thúc
    // Định dạng: HISTORY_END
    SendResponse(senderSocket, "HISTORY_END");
    DEBUG_LOG(L"Hoàn tất gửi lịch sử chat cho Client [%d]", (int)senderSocket);
}
