// RegLogin.cpp

#include "RegLogin.h"
#include "DbConnect.h"
#include "DebugLog.h"
#include <windows.h>
#include <wincrypt.h>
#include <sstream>
#include <iomanip>

// Hàm hỗ trợ Hash SHA-256
std::string HashPassword(const std::string& password) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE pbHash[32];
    DWORD dwDataLen = 32;
    std::string result = "";

    // 1. Acquire Context
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return ""; // Lỗi
    }

    // 2. Create Hash Object
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }

    // 3. Hash Data
    if (!CryptHashData(hHash, (BYTE*)password.c_str(), (DWORD)password.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }

    // 4. Get Hash Value
    if (CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &dwDataLen, 0)) {
        // Convert byte array to hex string
        std::stringstream ss;
        for (DWORD i = 0; i < dwDataLen; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)pbHash[i];
        }
        result = ss.str();
    }

    // 5. Clean up
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return result;
}

// check existed username
bool UserExists(const std::string& username)
{
    if (!g_db) return false;

    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT COUNT(1) FROM Users WHERE Username = ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        DEBUG_LOG(L"Lỗi prepare (UserExists): %S", sqlite3_errmsg(g_db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    // Execute
    int count = 0;
    rc = sqlite3_step(stmt); //
    if (rc == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0); // 
    }

    // Clean
    sqlite3_finalize(stmt);

    return (count > 0);
}


AuthResponse ProcessRegistration(const std::string& username, const std::string& password)
{
    if (username.empty() || password.empty()) {
        return AuthResponse::FAILED_INVALID_INPUT;
    }

    if (!g_db) {
        return AuthResponse::FAILED_DB_ERROR;
    }

    if (UserExists(username)) {
        return AuthResponse::FAILED_USER_EXISTS;
    }

    // INSERT
    sqlite3_stmt* stmt = NULL;
    const char* sql = "INSERT INTO Users (Username, PasswordHash, CreateDate) VALUES (?, ?, datetime('now', 'localtime'));";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        DEBUG_LOG(L"Lỗi prepare (Register): %S", sqlite3_errmsg(g_db));
        return AuthResponse::FAILED_DB_ERROR;
    }

    std::string hashedPassword = HashPassword(password); // Hash pass
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashedPassword.c_str(), -1, SQLITE_STATIC); // Lưu hash

    // Execute
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        DEBUG_LOG(L"Lỗi step (Register): %S", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return AuthResponse::FAILED_DB_ERROR;
    }

    // Clean
    sqlite3_finalize(stmt);

    DEBUG_LOG(L"Đăng ký thành công user: %S", username.c_str());
    return AuthResponse::SUCCESS_REGISTER;
}


AuthResponse ProcessLogin(const std::string& username, const std::string& password)
{
    if (username.empty() || password.empty()) {
        return AuthResponse::FAILED_INVALID_INPUT;
    }

    if (!g_db) {
        return AuthResponse::FAILED_DB_ERROR;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT UserId, PasswordHash FROM Users WHERE Username = ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        DEBUG_LOG(L"Lỗi prepare (Login): %S", sqlite3_errmsg(g_db));
        return AuthResponse::FAILED_DB_ERROR;
    }

    // 
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    // Execute
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        int userId = sqlite3_column_int(stmt, 0);
        const char* dbPasswordHash = (const char*)sqlite3_column_text(stmt, 1); // Lấy hash từ DB

        std::string inputHash = HashPassword(password); // Hash pass người dùng nhập

        if (inputHash == std::string(dbPasswordHash)) { // So sánh 2 chuỗi hash
            sqlite3_finalize(stmt);
            DEBUG_LOG(L"Đăng nhập thành công user: %S", username.c_str());
            return AuthResponse::SUCCESS_LOGIN;
        }
        else {
            sqlite3_finalize(stmt);
            DEBUG_LOG(L"Đăng nhập thất bại (sai pass) user: %S", username.c_str());
            return AuthResponse::FAILED_WRONG_PASS;
        }
    }
    else if (rc == SQLITE_DONE)
    {
        // Không có dòng nào -> User không tồn tại
        sqlite3_finalize(stmt);
        DEBUG_LOG(L"Đăng nhập thất bại (không tìm thấy) user: %S", username.c_str());
        return AuthResponse::FAILED_NOT_FOUND;
    }
    else
    {
        DEBUG_LOG(L"Lỗi step (Login): %S", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return AuthResponse::FAILED_DB_ERROR;
    }
}