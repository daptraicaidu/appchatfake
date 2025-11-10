// DebugLog.cpp

#include "DebugLog.h"
#include <sstream>      
#include <iomanip>      
#include <stdio.h>      
#include <stdarg.h>     


void LogToDebugView(const WCHAR* fmt, ...)
{
    WCHAR user_message[1024];

    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(user_message, _countof(user_message), _TRUNCATE, fmt, args);
    va_end(args);

    SYSTEMTIME st; GetLocalTime(&st);
    std::wstringstream wss;

    wss << L"[" << std::setw(2) << std::setfill(L'0') << st.wHour
        << L":" << std::setw(2) << std::setfill(L'0') << st.wMinute
        << L":" << std::setw(2) << std::setfill(L'0') << st.wSecond
        << L"] [Database] "
        << user_message
        << L"\n";

    OutputDebugStringW(wss.str().c_str());
}
