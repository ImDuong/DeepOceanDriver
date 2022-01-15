#pragma once

#include "pch.h"
#include "helpers.h"
#include <vector>

int Error(const char* message) {
    printf("%s (error=%d)\n", message, GetLastError());
    return 1;
}

void DisplayTime(const LARGE_INTEGER& time) {
    SYSTEMTIME st;
    ::FileTimeToSystemTime((FILETIME*)&time, &st);
    printf("%02d:%02d:%02d.%03d: ",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void DisplayBinary(const UCHAR* buffer, DWORD size) {
    for (DWORD i = 0; i < size; i++)
        printf("%02X ", buffer[i]);
    printf("\n");
}

void TcPrintUsage()
{
    puts("Usage:");
    puts("");
    puts("    CyStack_Endpoint_Launcher.exe [-?]");
    puts("     -procnoti                                get notification of processes");
}
