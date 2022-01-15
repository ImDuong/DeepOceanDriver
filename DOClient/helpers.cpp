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

void DOPrintInstruction()
{
    puts("Welcome to Deep Ocean Service. We're excited to show you a whole new world under the water. Together, we will discover the forbidden treasures. But first, let's take a look at DeOc's instruction.");
    puts("");
    puts("Usage:");
    puts("    DOClient.exe [-?]");
    puts("     -procnoti                                get notification of processes");
}
