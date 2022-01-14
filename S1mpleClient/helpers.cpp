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

std::vector<DWORD> ParsePids(const wchar_t* buffer[], int count) {
    std::vector<DWORD> pids;
    for (int i = 0; i < count; i++)
        pids.push_back(::_wtoi(buffer[i]));
    return pids;
}

void TcPrintUsage()
{
    puts("Usage:");
    puts("");
    puts("    CyStack_Endpoint_Launcher.exe [-?]");
    puts("     -prioritybooster <threadid> <priority>   change priority of a thread");
    puts("     -zerobuffer                              read and write a buffer");
    puts("     -zerostats                               get zero buffer mode info");
    puts("     -procnoti                                get notification of processes");
    puts("     -procprot <pid1> <pid2> .... <pidn>      get notification of processes");
    puts("     -procunprot <pid1> <pid2> .... <pidn>    remove protection for processes");
    puts("     -procprotclear                           clear process protection");
}
