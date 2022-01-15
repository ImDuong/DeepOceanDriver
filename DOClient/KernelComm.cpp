#include "pch.h"
#include "kernelcomm.h"
#include "helpers.h"
#include "datastore.h"
#include "..\DODriver\drivercommon.h"

HANDLE hDevice;

HANDLE GetDeviceHandle() {
    return hDevice;
}

int OpenDevice() {
    // open handle to Deep Ocean Device
    // this API reaches IRP_MJ_CREATE dispatch routine of driver
    hDevice = CreateFile(DEOC_DEVICE, GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice == INVALID_HANDLE_VALUE)
        return Error("Failed to open device");
    std::cout << "opening " << DEOC_DEVICE_NAME << " device...\n";
    return 0;
}

int CloseDevice() {
    if (hDevice) {
        CloseHandle(hDevice);
        std::cout << "closing " << DEOC_DEVICE_NAME << " device...\n";
    }
    return 0;
}

// this function allow client to read information with buffer without checking whether read buffer size is different from expected maximum buffer size -> loose read technique
int KernelLooseRead(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead) {
    //printf("Address stored in buffer after passing: %p\n", lpBuffer);
    BOOL ok = ::ReadFile(hDevice, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, nullptr);
    if (!ok)
        return Error("failed to read");
    return 0;
}