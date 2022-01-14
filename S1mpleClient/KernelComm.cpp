#include "pch.h"
#include "KernelComm.h"
#include "helpers.h"
#include "..\S1mpleDriver\S1mpleCommon.h"

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

int KernelPriorityBooster(int threadId, int priority) {
    ThreadData data;
    data.ThreadId = threadId;
    data.Priority = priority;
    /*data.ThreadId = atoi(argv[1]);
    data.Priority = atoi(argv[2]);*/

    // call IRP_MJ_DEVICE_CONTROL of driver
    DWORD returned;
    BOOL success = DeviceIoControl(hDevice,
        IOCTL_S1MPLE_SET_PRIORITY, // control code
        &data, sizeof(data), // input buffer and length
        nullptr, 0, // output buffer and length
        &returned, nullptr);
    if (success)
        printf("Priority change succeeded!\n");
    else
        return Error("Priority change failed!");
    return 0;
}

ZeroStats KernelGetZeroStats() {
    // prepare data to get output from driver
    ZeroStats data;

    // call IRP_MJ_DEVICE_CONTROL of driver
    DWORD returned;
    BOOL success = DeviceIoControl(hDevice,
        IOCTL_S1MPLE_GET_ZERO_STATS, // control code
        nullptr, 0, // input buffer and length
        &data, sizeof(data), // output buffer and length
        &returned, nullptr);
    if (success) {
        printf("Get zero stats succeeded!\n");
    }
    else {
        Error("Get zero stats failed!");
    }
    return data;
}

int KernelRead(LPVOID lpBuffer, DWORD nNumberOfBytesToRead) {
    DWORD bytes;
    //printf("Address stored in buffer after passing: %p\n", lpBuffer);
    BOOL ok = ::ReadFile(hDevice, lpBuffer, nNumberOfBytesToRead, &bytes, nullptr);
    if (!ok)
        return Error("failed to read");
    if (bytes != nNumberOfBytesToRead)
        return Error("Wrong number of bytes\n");
    return 0;
}

int KernelWrite(LPVOID lpBuffer, DWORD nNumberOfBytesToWrite) {
    DWORD bytes;
    BOOL ok = ::WriteFile(hDevice, lpBuffer, nNumberOfBytesToWrite, &bytes, nullptr);
    if (!ok)
        return Error("failed to write");
    if (bytes != nNumberOfBytesToWrite)
        return Error("Wrong byte count\n");
    return 0;
}

int KernelLooseRead(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead) {
    //printf("Address stored in buffer after passing: %p\n", lpBuffer);
    BOOL ok = ::ReadFile(hDevice, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, nullptr);
    if (!ok)
        return Error("failed to read");
    return 0;
}

int KernelProtectProcessByPID(LPVOID lpBuffer, DWORD nBufferSize) {
    DWORD bytes;
    BOOL success = DeviceIoControl(hDevice,
        IOCTL_S1MPLE_PROTECT_PROCESS_BY_PID, // control code
        lpBuffer, nBufferSize, // input buffer and length
        nullptr, 0, // output buffer and length
        &bytes, nullptr);
    if (success) {
        printf("Protect process by PID successfully!\n");
    }
    else {
        return Error("Send IOCTL to driver failed!");
    }
    return 0;
}

int KernelUnprotectProcessByPID(LPVOID lpBuffer, DWORD nBufferSize) {
    DWORD bytes;
    BOOL success = DeviceIoControl(hDevice,
        IOCTL_S1MPLE_UNPROTECT_PROCESS_BY_PID, // control code
        lpBuffer, nBufferSize, // input buffer and length
        nullptr, 0, // output buffer and length
        &bytes, nullptr);
    if (success) {
        printf("Unprotect process by PID successfully!\n");
    }
    else {
        return Error("Send IOCTL to driver failed!");
    }
    return 0;
}

int KernelClearProcessProtection() {
    DWORD bytes;
    BOOL success = DeviceIoControl(hDevice,
        IOCTL_S1MPLE_CLEAR_PROTECT_PROCESS, // control code
        nullptr, 0, // input buffer and length
        nullptr, 0, // output buffer and length
        &bytes, nullptr);
    if (success) {
        printf("Clear process protection successfully!\n");
    }
    else {
        return Error("Send IOCTL to driver failed!");
    }
    return 0;
}

int KernelProtectRegistryByPID(LPVOID lpBuffer, DWORD nBufferSize) {
    DWORD bytes;
    BOOL success = DeviceIoControl(hDevice,
        IOCTL_S1MPLE_REGKEY_PROTECT_ADD, // control code
        lpBuffer, nBufferSize, // input buffer and length
        nullptr, 0, // output buffer and length
        &bytes, nullptr);
    if (success) {
        printf("Protect registry successfully!\n");
    }
    else {
        return Error("Send IOCTL to driver failed!");
    }
    return 0;
}

int KernelUnProtectRegistryByPID(LPVOID lpBuffer, DWORD nBufferSize) {
    DWORD bytes;
    BOOL success = DeviceIoControl(hDevice,
        IOCTL_S1MPLE_REGKEY_PROTECT_REMOVE, // control code
        lpBuffer, nBufferSize, // input buffer and length
        nullptr, 0, // output buffer and length
        &bytes, nullptr);
    if (success) {
        printf("Unprotect registry successfully!\n");
    }
    else {
        return Error("Send IOCTL to driver failed!");
    }
    return 0;
}