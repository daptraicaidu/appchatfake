//DbConnect.cpp

#include "DbConnect.h"
#include <windows.h> 
#include "DebugLog.h"
#include <debugapi.h>

sqlite3* g_db = NULL;


bool ConnectDatabase()
{
    if (g_db) {
        return true;
    }

    const char* dbPath = "C:\\ProgramData\\ChatAppService\\chat.db";


    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX;

    // Gọi hàm mở DB của SQLite
    int rc = sqlite3_open_v2(dbPath, &g_db, flags, NULL);

    if (rc != SQLITE_OK)
    {
        DEBUG_LOG(L"Cannot open DB: %S", sqlite3_errmsg(g_db));
        if (g_db) sqlite3_close(g_db);
        g_db = NULL;
        return false;
    }

    const char* pragmas =
        "PRAGMA journal_mode=WAL;"   
        "PRAGMA foreign_keys=ON;"    
        "PRAGMA synchronous=NORMAL;"; 

    char* errMsg = NULL;
    rc = sqlite3_exec(g_db, pragmas, NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        DEBUG_LOG(L"Cannot setup PRAGMA: %S", errMsg);
        sqlite3_free(errMsg);
    }

    DEBUG_LOG(L"SQLite DB connected");

    return true;
}


void DisconnectDatabase()
{
    if (g_db)
    {
        sqlite3_close(g_db);
        g_db = NULL;
        DEBUG_LOG(L"Đã ngat ket noi SQLite DB.");
    }
}
