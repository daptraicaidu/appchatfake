// -------- Nội dung file DebugLog.h --------


#pragma once

#include <windows.h> 


void LogToDebugView(const WCHAR* fmt, ...);


#define DEBUG_LOG(fmt, ...) LogToDebugView(fmt, ##__VA_ARGS__)