#pragma once
#include <windows.h>
#include <stdio.h>

static inline void DebugLog(const wchar_t* fmt, ...) {
    wchar_t buf[1024];

    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);

    OutputDebugStringW(buf);
    OutputDebugStringW(L"\r\n");
}

// Macro cho tiện
#define DEBUG_LOG(fmt, ...) DebugLog(L"" fmt, __VA_ARGS__)
