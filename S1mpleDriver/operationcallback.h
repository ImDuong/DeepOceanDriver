#pragma once

// explicity define access mask because there's no PROCESS_TERMINATE in wdk header
// to set the right value for the right access mask, view this link: https://docs.microsoft.com/en-us/windows/win32/procthread/process-security-and-access-rights
#define PROCESS_TERMINATE 1

OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION Info);


bool FindProcess(ULONG pid);

bool AddProcess(ULONG pid);

bool RemoveProcess(ULONG pid);