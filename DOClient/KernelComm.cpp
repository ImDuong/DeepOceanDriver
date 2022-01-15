#include "pch.h"
#include "KernelComm.h"
#include "helpers.h"
#include "..\DODriver\S1mpleCommon.h"

HANDLE hDevice;

HANDLE GetDeviceHandle() {
    return hDevice;
}

int OpenDevice() {
    // open handle to S1mple Device
    // this API reaches IRP_MJ_CREATE dispatch routine of driver
    hDevice = CreateFile(L"\\\\.\\S1mpleDevice", GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice == INVALID_HANDLE_VALUE)
        return Error("Failed to open device");
    printf("opening device...\n");
    return 0;
}

int CloseDevice() {
    if (hDevice) {
        CloseHandle(hDevice);
        printf("closing device...");
    }
    return 0;
}

int KernelLooseRead(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead) {
    //printf("Address stored in buffer after passing: %p\n", lpBuffer);
    BOOL ok = ::ReadFile(hDevice, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, nullptr);
    if (!ok)
        return Error("failed to read");
    return 0;
}