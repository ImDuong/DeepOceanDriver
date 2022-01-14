#include "pch.h"
#include "helpers.h"
#include "KernelComm.h"
#include "..\S1mpleDriver\S1mpleCommon.h"
#include <vector>

enum class Options {
    Unknown,
    ProtectPid, UnProtectPid, ClearProtectPid,
    ProtectReg, UnProtectReg, ClearProtectReg
};

bool PriorityBooster(int threadId, int priority) {
    /*if (argc < 3) {
        std::cout << "Usage: S1mpleClient <threadid> <priority>\n";
        return false;
    }*/
    if (OpenDevice()) {
        return false;
    }

    if (KernelPriorityBooster(threadId, priority)) {
        return false;
    }

    if (CloseDevice()) {
        return false;
    }
    return true;
}

bool GetZeroStats() {
    /*if (argc < 3) {
        std::cout << "Usage: S1mpleClient <threadid> <priority>\n";
        return false;
    }*/
    std::cout << "starting...\n";
    if (OpenDevice()) {
        std::cout << "Open device failed";
        return false;
    }

    std::cout << "Getting zero stats\n";
    ZeroStats data = KernelGetZeroStats();
    std::cout << "\nGot zero stats";
    std::cout << "\nTotal read bytes: " << data.TotalRead << std::endl;
    std::cout << "\nClosing\n";
    /*if (KernelGetZeroStats()) {
        std::cout << "Get zero stats failed";
        return false;
    }*/

    if (CloseDevice()) {
        std::cout << "Close device failed";
        return false;
    }
    return true;
}

bool ZeroBuffer() {
    if (OpenDevice()) {
        return false;
    }
    
    ///////////////////////// test read
    BYTE buffer[64];
    // store some non-zero data
    for (int i = 0; i < sizeof(buffer); ++i)
        buffer[i] = i + 1;
    /*BYTE* pBuf = (BYTE*)buffer;
    for (int i = 0; i < sizeof(buffer); i++) {
        std::cout << *(pBuf + i) << "; ";
    }*/

    KernelRead(buffer, sizeof(buffer));
    // check if buffer data sum is zero
    long total = 0;
    for (auto n : buffer) {
        //std::cout << n;
        total += n;
    }
    //std::cout << std::endl;
    if (total != 0)
        printf("Wrong data\n");
    else {
        printf("Read correct data\n");
    }

    ///////////////////////// test write
    BYTE buffer2[1024]; // contains junk
    KernelWrite(buffer2, sizeof(buffer2));

    if (CloseDevice()) {
        return false;
    }
    return true;
}

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
        case ItemType::ThreadExit:
        {
            DisplayTime(header->Time);
            auto info = (ThreadCreateExitInfo*)buffer;
            printf("Thread %d Exited from Process %d\n", info->ThreadId, info->ProcessId);
            break;
        }
        case ItemType::ThreadCreate:
        {
            DisplayTime(header->Time);
            auto info = (ThreadCreateExitInfo*)buffer;
            printf("Thread %d Created from Process %d\n", info->ThreadId, info->ProcessId);
            break;
        }
        case ItemType::ImageLoad:
        {
            DisplayTime(header->Time);
            auto info = (ImageLoadInfo*)buffer;
            printf("Image loaded into process %d at address 0x%p (%ws)\n", info->ProcessId, info->LoadAddress, info->ImageFileName);
            break;
        }
        case ItemType::RegistrySetValue:
        {
            DisplayTime(header->Time);
            auto info = (RegistrySetValueInfo*)buffer;
            printf("Registry write PID=%d: %ws\\%ws type: %d size: %d data: ", info->ProcessId,
                info->KeyName, info->ValueName, info->DataType, info->DataSize);
            switch (info->DataType) {
            case REG_DWORD:
                printf("0x%08X\n", *(DWORD*)info->Data);
                break;

            case REG_SZ:
            case REG_EXPAND_SZ:
                printf("%ws\n", (WCHAR*)info->Data);
                break;

            case REG_BINARY:
                DisplayBinary(info->Data, min(info->DataSize, sizeof(info->Data)));
                break;

            default:
                DisplayBinary(info->Data, min(info->DataSize, sizeof(info->Data)));
                break;

            }

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

bool MonitorProcessProtection(Options option, std::vector<DWORD> pids) {
    if (OpenDevice()) {
        return false;
    }

    switch (option) {
        case Options::ProtectPid: {
            KernelProtectProcessByPID(pids.data(), static_cast<DWORD>(pids.size()) * sizeof(DWORD));
            break;
        }
        case Options::UnProtectPid: {
            KernelUnprotectProcessByPID(pids.data(), static_cast<DWORD>(pids.size()) * sizeof(DWORD));
            break;
        }
        case Options::ClearProtectPid: {
            KernelClearProcessProtection();
            break;
        }
    }
    if (CloseDevice()) {
        return false;
    }
    return true;
}

bool MonitorRegistryProtection(Options option, const wchar_t* registry_path) {
    if (OpenDevice()) {
        return false;
    }

    switch (option) {
    case Options::ProtectReg: {
        KernelProtectRegistryByPID((PVOID)registry_path, ((DWORD)::wcslen(registry_path) + 1) * sizeof(WCHAR));
        break;
    }
    case Options::UnProtectReg: {
        KernelUnProtectRegistryByPID((PVOID)registry_path, ((DWORD)::wcslen(registry_path) + 1) * sizeof(WCHAR));
        break;
    }
    case Options::ClearProtectReg: {
        
        break;
    }
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
        else if (0 == ::_wcsicmp(arg, L"-prioritybooster")) {
            if (argc == 4) {
                int threadId = ::_wtoi(argv[2]);
                int priority = ::_wtoi(argv[3]);
            
                if (!PriorityBooster(threadId, priority)) {
                    ExitCode = ERROR_FUNCTION_FAILED;
                }
            }
            else {
                ExitCode = ERROR_FUNCTION_FAILED;
            }
        }
                    
        else if (0 == ::_wcsicmp(arg, L"-zerobuffer")) {
            if (!ZeroBuffer()) {
                ExitCode = ERROR_FUNCTION_FAILED;
            }
        }
                        
        else if (0 == ::_wcsicmp(arg, L"-zerostats")) {
            if (!GetZeroStats()) {
                ExitCode = ERROR_FUNCTION_FAILED;
            }
        }
        else if (0 == ::_wcsicmp(arg, L"-procnoti")) {
            if (!PollProcessNotification()) {
                ExitCode = ERROR_FUNCTION_FAILED;
            }
        }
        else if (0 == ::_wcsicmp(arg, L"-procprot")) {
            auto option = Options::ProtectPid;
            std::vector<DWORD> pids = ParsePids(argv + 2, argc - 2);
            if (!MonitorProcessProtection(option, pids)) {
                ExitCode = ERROR_FUNCTION_FAILED;
            }
        }
        else if (0 == ::_wcsicmp(arg, L"-regprot")) {
            auto option = Options::ProtectReg;
            if (!MonitorRegistryProtection(option, argv[2])) {
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