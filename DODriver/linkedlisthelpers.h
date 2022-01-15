#pragma once
#include "fastmutex.h"
#include "drivercommon.h"

// note: Mt prefix indicates the mutex lock inside the function

void PushItem(__in LIST_ENTRY* entry, __in LIST_ENTRY* entryHead, __inout int& currentItemCount, __in int maxItemCount);

void MtPopItem(__in LIST_ENTRY* entry, __inout int& currentItemCount, FastMutex itemLock);

void MtViewItem(LIST_ENTRY* entryHead, ItemType itemType, int currentItemCount, FastMutex itemLock);

NTSTATUS FindItem(__in PUNICODE_STRING pTargetItem, __in LIST_ENTRY* entryHead, __in ItemType itemType, __in int currentItemCount, __out PLIST_ENTRY* foundEntry);