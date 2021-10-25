#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00	// Windows 10 ( https://docs.microsoft.com/en-us/windows/win32/winprog/using-the-windows-headers )
#include <windows.h>
#include <strsafe.h>
#include <shellapi.h>
#include <mmsystem.h>

#include <stdlib.h>

#include <functional>
#include <array>

#define APPNAME    "kbdacc"
#define APPNAME_L L"kbdacc"

#if defined(_WIN64)
#  if !defined(_DEBUG)
#    define DLL_NAME    "kbdacc_dll.dll"    // x64, release
#  else
#    define DLL_NAME    "kbdacc_dllD.dll"   // x64, debug
#  endif
#else
#  if !defined(_DEBUG)
#    define DLL_NAME    "kbdacc_dll.dll32"  // x86, release
#  else
#    define DLL_NAME    "kbdacc_dllD.dll32" // x86, debug
#  endif
#endif


#if defined(_DEBUG)
inline void outputDebugString(const wchar_t* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    wchar_t buf[1024];
    vswprintf_s(buf, std::size(buf), fmt, args);
    ::OutputDebugStringW(buf);
    va_end(args);
}
inline void outputDebugString(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[1024];
    vsprintf_s(buf, std::size(buf), fmt, args);
    OutputDebugStringA(buf);
    va_end(args);
}
#else
#define outputDebugString(...)
#define outputDebugString(...)
#endif
