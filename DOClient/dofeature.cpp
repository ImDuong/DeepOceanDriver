#include "pch.h"
#include "../DODriver/drivercommon.h"
#include "kernelgateway.h"
#include "dofeature.h"
#include "helpers.h"

void DisplayNotification(BYTE* buffer, DWORD size) {
    auto count = size;
    while (count > 0) {
        auto header = (ItemHeader*)buffer;
        switch (header->Type) {
        case ItemType::ProcessCreate:
        {
            // display noti time
            DisplayTime(header->Time);
            // extract info of noti from buffer
            auto info = (ProcessCreateInfo*)buffer;
            // command line is located a (command line offset) size starting from buffer base address
            std::wstring commandline((WCHAR*)(buffer + info->CommandLineOffset), info->CommandLineLength);
            // display noti in console
            printf("Process %d Created. Command line: %ws\n", info->ProcessId,
                commandline.c_str());
            // todo: display noti for registerd process in popup (Windows 10 - powershell)
            break;
        }
        case ItemType::ProcessExit:
        {
            // display noti time
            DisplayTime(header->Time);
            // extract info of noti from buffer
            auto info = (ProcessExitInfo*)buffer;
            // display noti in console
            printf("Process %d Exited\n", info->ProcessId);
            // todo: display noti for registerd process in popup (Windows 10 - powershell)
            break;
        }
        default:
            break;
        }
        // increase the buffer size by the bytes we just read
        buffer += header->Size;
        // decrease the bytes we just read
        count -= header->Size;
    }
}

bool PollNotiFromProc(KernelGateway* krnlgw) {
    if (!krnlgw->IsGatewayOk()) {
        std::cout << "Cannot open device to kernel\n";
        return false;
    }

    // we won't read the noti forever -> we assign a specific buffer size to read noti from kernel
    BYTE buffer[1 << 16]; // 64KB buffer

    while (true) {
        DWORD bytes;
        if (krnlgw->LooseReadFromKernel(buffer, sizeof(buffer), &bytes)) {
            // display notification
            DisplayNotification(buffer, bytes);
        }
        ::Sleep(200);
    }

    return true;
}