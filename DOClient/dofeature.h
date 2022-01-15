#pragma once

#include "kernelgateway.h"

void DisplayNotification(BYTE* buffer, DWORD size);

bool PollNotiFromProc(KernelGateway* krnlgw);