#include "pch.h"
#include "kernelgateway.h"
#include "helpers.h"
#include "datastore.h"
#include "..\DODriver\drivercommon.h"

KernelGateway::KernelGateway(const wchar_t* deviceName)
{
    printf("Device Name: %ws\n", deviceName);
    KernelGateway::OpenDevice(deviceName);
}

KernelGateway::~KernelGateway()
{
    KernelGateway::CloseDevice();
}

bool KernelGateway::IsGatewayOk() {
    if (KernelGateway::hDevice == INVALID_HANDLE_VALUE)
        return false;
    return true;
}

bool KernelGateway::OpenDevice(const wchar_t* deviceName) {
    // open handle to Deep Ocean Device
    // this API reaches IRP_MJ_CREATE dispatch routine of driver
    KernelGateway::hDevice = CreateFile(deviceName, GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (KernelGateway::hDevice == INVALID_HANDLE_VALUE)
        return false;
    printf("opening device...\n");
    return true;
}

bool KernelGateway::CloseDevice() {
    if (KernelGateway::hDevice) {
        CloseHandle(KernelGateway::hDevice);
        printf("closing device...");
    }
    return true;
}

bool KernelGateway::SendDataToKernel(Options option, LPVOID lpBuffer, DWORD nBufferSize) {
    DWORD bytes;
    switch (option) {
    // serious problem right here: the caller of this func send the size of the wchar msg, not the size of the struct -> redo the caller
    case Options::BlockProg: {
        BOOL success = DeviceIoControl(KernelGateway::hDevice,
            IOCTL_DO_PROGRAM_BLOCK, // control code
            lpBuffer, nBufferSize, // input buffer and length
            nullptr, 0, // output buffer and length
            &bytes, nullptr);
        if (success) {
            printf("Block program path successfully!\n");
        }
        else {
            Error("Send IOCTL to driver failed!");
            return false;
        }
        break;
    }
    case Options::UnBlockProg: {
        BOOL success = DeviceIoControl(KernelGateway::hDevice,
            IOCTL_DO_PROGRAM_UNBLOCK, // control code
            lpBuffer, nBufferSize, // input buffer and length
            nullptr, 0, // output buffer and length
            &bytes, nullptr);
        if (success) {
            printf("Unblock program path successfully!\n");
        }
        else {
            Error("Send IOCTL to driver failed!");
            return false;
        }
        break;
    }
    case Options::ClearBlockProg: {
        BOOL success = DeviceIoControl(KernelGateway::hDevice,
            IOCTL_DO_PROGRAM_UNBLOCK, // control code
            lpBuffer, nBufferSize, // input buffer and length
            nullptr, 0, // output buffer and length
            &bytes, nullptr);
        if (success) {
            printf("Clear blocking program list successfully!\n");
        }
        else {
            Error("Send IOCTL to driver failed!");
            return false;
        }
        break;
    }
    default: {
        printf("Unknown device io control option!\n");
        return false;
    }
    }
    return true;
}

// this function allow client to read information with buffer without checking whether read buffer size is different from expected maximum buffer size -> loose read technique
bool KernelGateway::LooseReadFromKernel(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead) {
    BOOL ok = ::ReadFile(KernelGateway::hDevice, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, nullptr);
    if (!ok) {
        Error("failed to read");
        return false;
    }
    return true;
}