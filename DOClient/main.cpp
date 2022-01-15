#include "pch.h"
#include "helpers.h"
#include "kernelgateway.h"
#include "..\DODriver\drivercommon.h"
#include "dofeature.h"
#include "datastore.h"
#include <vector>



int _cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) const wchar_t* argv[]
)
{
    int ExitCode = ERROR_SUCCESS;
    if (argc > 1)
    {
        KernelGateway* krnlgw = new KernelGateway(DEOC_DEVICE);

        if (!krnlgw->IsGatewayOk()) {
            std::cout << "Cannot open device to kernel\n";
            ExitCode = ERROR_ERRORS_ENCOUNTERED;
        }
        else {
            const wchar_t* arg = argv[1];
            if ((0 == ::_wcsicmp(arg, L"-?")) || (0 == ::_wcsicmp(arg, L"-h")) || (0 == ::_wcsicmp(arg, L"-help"))) {
                DOPrintInstruction();
            }
            else if (0 == ::_wcsicmp(arg, L"-procnoti")) {
                if (!PollNotiFromProc(krnlgw)) {
                    ExitCode = ERROR_FUNCTION_FAILED;
                }
            }
            else if (0 == ::_wcsicmp(arg, L"-progblock") || 0 == ::_wcsicmp(arg, L"-progunblock")) {
                wchar_t dosProcPath[MaxProcessPathSize];
                wcsncpy_s(dosProcPath, MaxProcessPathSize, argv[2], wcslen(argv[2]));
                // we just need to send the driver the dos path
            
                // basic path validation
                // check if process path valid or not
                if (::wcslen(dosProcPath) < 3) {
                    printf("Process path is too small\n");
                    return ERROR_PATH_NOT_FOUND;
                }

                // check if our path starts with `X:\`
                if (dosProcPath[1] != L':' || dosProcPath[2] != L'\\') {
                    printf("Process path is not valid\n");
                    return ERROR_PATH_NOT_FOUND;
                }

                Options option;
                if (0 == ::_wcsicmp(arg, L"-progblock")) {
                    option = Options::BlockProg;
                }
                else {
                    option = Options::UnBlockProg;
                }

                krnlgw->SendDataToKernel(option, (PVOID)dosProcPath, ((DWORD)::wcslen(dosProcPath) + 1) * sizeof(WCHAR));
            }

        }
    }
    else
    {
        DOPrintInstruction();
    }
Exit:
    return ExitCode;
}