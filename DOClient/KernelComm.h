#pragma once

HANDLE getDeviceHandle();
int OpenDevice();

int CloseDevice();

int KernelLooseRead(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead);