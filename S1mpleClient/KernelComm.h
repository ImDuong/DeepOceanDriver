#pragma once

HANDLE getDeviceHandle();
int OpenDevice();

int CloseDevice();

int KernelPriorityBooster(int threadId, int priority);

struct ZeroStats KernelGetZeroStats();

int KernelRead(LPVOID lpBuffer, DWORD nNumberOfBytesToRead);

int KernelLooseRead(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead);

int KernelWrite(LPVOID lpBuffer, DWORD nNumberOfBytesToWrite);

int KernelProtectProcessByPID(LPVOID lpBuffer, DWORD nBufferSize);

int KernelUnprotectProcessByPID(LPVOID lpBuffer, DWORD nBufferSize);

int KernelClearProcessProtection();

int KernelProtectRegistryByPID(LPVOID lpBuffer, DWORD nBufferSize);

int KernelUnProtectRegistryByPID(LPVOID lpBuffer, DWORD nBufferSize);