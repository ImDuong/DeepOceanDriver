#include "pch.h"
#include "operationcallback.h"
#include "sysmon.h"
#include "autolock.h"

extern Globals g_Globals;

OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID /* RegistrationContext */, POB_PRE_OPERATION_INFORMATION Info) {
    // don't stop kernel code from working properly
    if (Info->KernelHandle)
        return OB_PREOP_SUCCESS;

    auto process = (PEPROCESS)Info->Object;
    auto pid = HandleToULong(PsGetProcessId(process));

    AutoLock locker(g_Globals.ProcProtLock);
    if (FindProcess(pid)) {
        // found in list, remove terminate access mask
        KdPrint(("Found the blacklist pid: %d\n", pid));
        Info->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
    }

    return OB_PREOP_SUCCESS;
}

// these helpers do not acquire mutex -> caller needs to acquire mutex before invoking these helpers

bool FindProcess(ULONG pid) {
    // if we don't store any pid -> quit immediately
    if (g_Globals.PidsCount == 0) {
        return false;
    }

    for (int i = 0; i < MaxPids; i++)
        if (g_Globals.Pids[i] == pid)
            return true;
    return false;
}

bool AddProcess(ULONG pid) {
    for (int i = 0; i < MaxPids; i++)
        if (g_Globals.Pids[i] == 0) {
            // empty slot
            g_Globals.Pids[i] = pid;
            g_Globals.PidsCount++;
            return true;
        }
    return false;
}

bool RemoveProcess(ULONG pid) {
    for (int i = 0; i < MaxPids; i++)
        if (g_Globals.Pids[i] == pid) {
            g_Globals.Pids[i] = 0;
            g_Globals.PidsCount--;
            return true;
        }
    return false;
}
