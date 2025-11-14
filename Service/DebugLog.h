#pragma once
#include <windows.h>
#include <stdio.h>      // _vsnwprintf_s

// In 1 dòng ra OutputDebugStringW (Unicode), có xuống dòng luôn.
static inline void DebugLog(const wchar_t* fmt, ...) {
    wchar_t buf[1024];

    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, ap); // format trước
    va_end(ap);

    OutputDebugStringW(buf);
    OutputDebugStringW(L"\r\n"); // CRLF để DebugView dễ đọc
}

// Macro cho tiện
#define DEBUG_LOG(fmt, ...) DebugLog(L"" fmt, __VA_ARGS__)
