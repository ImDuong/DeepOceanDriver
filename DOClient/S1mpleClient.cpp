#include "pch.h"
#include "helpers.h"
#include "KernelComm.h"
#include "..\DODriver\S1mpleCommon.h"
#include <vector>

void DisplayInfo(BYTE* buffer, DWORD size) {
    auto count = size;
    while (count > 0) {
        auto header = (ItemHeader*)buffer;
        switch (header->Type) {
        case ItemType::ProcessExit:
        {
            DisplayTime(header->Time);
            auto info = (ProcessExitInfo*)buffer;
            printf("Process %d Exited\n", info->ProcessId);
            break;
        }
        case ItemType::ProcessCreate:
        {
            DisplayTime(header->Time);
            auto info = (ProcessCreateInfo*)buffer;
            std::wstring commandline((WCHAR*)(buffer + info->CommandLineOffset), info->CommandLineLength);
            printf("Process %d Created. Command line: %ws\n", info->ProcessId,
                commandline.c_str());
            break;
        }
        default:
            break;
        }
        buffer += header->Size;
        count -= header->Size;
    }
}

bool PollProcessNotification() {
    if (OpenDevice()) {
        return false;
    }

    BYTE buffer[1 << 16]; // 64KB buffer

    while (true) {
        DWORD bytes;
        if (!KernelLooseRead(buffer, sizeof(buffer), &bytes)) {
            DisplayInfo(buffer, bytes);
        }
        ::Sleep(200);
    }

    if (CloseDevice()) {
        return false;
    }
    return true;
}

int _cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) const wchar_t* argv[]
)
{
    int ExitCode = ERROR_SUCCESS;
    if (argc > 1)
    {
        const wchar_t* arg = argv[1];
        if ((0 == ::_wcsicmp(arg, L"-?")) || (0 == ::_wcsicmp(arg, L"-h")) || (0 == ::_wcsicmp(arg, L"-help"))) {
            TcPrintUsage();
        }
        else if (0 == ::_wcsicmp(arg, L"-procnoti")) {
            if (!PollProcessNotification()) {
                ExitCode = ERROR_FUNCTION_FAILED;
            }
        }
    }
    else
    {
        TcPrintUsage();
    }
Exit:
    return ExitCode;
}