#pragma once

#include <vector>

int Error(const char* message);

void DisplayTime(const LARGE_INTEGER& time);

void DisplayBinary(const UCHAR* buffer, DWORD size);

void TcPrintUsage();