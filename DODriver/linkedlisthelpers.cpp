#include "pch.h"
#include "linkedlisthelpers.h"
#include "datastore.h"
#include "autolock.h"
#include "fastmutex.h"
#include "drivercommon.h"
#include "commonhelpers.h"

// the caller must acquire lock for it & check for existence before pushing
void PushItem(__in LIST_ENTRY* entry, __in LIST_ENTRY* entryHead, __inout int& currentItemCount, __in int maxItemCount)
{
    if (currentItemCount > maxItemCount)
    {
        // too many items, remove oldest one
        auto head = RemoveHeadList(entryHead);
        currentItemCount--;
        ExFreePool(CONTAINING_RECORD(head, DOItem<ItemHeader*>, Entry));
    }

    //// get the path from inserted item
    //auto entryItem = CONTAINING_RECORD(entry, DOItem<RegistryKeyProtectInfo>, Entry);
    //if (entryItem == NULL || entryItem->Data.KeyName == NULL) {
    //    return;
    //}
    //auto insertedKName = entryItem->Data.KeyName;
    //UNICODE_STRING insertedRegPath;
    //RtlInitUnicodeString(&insertedRegPath, insertedKName);

    //// we already acquire the mutex -> safely call FindRegistryItem
    //PLIST_ENTRY foundEntry = nullptr;
    //if (!NT_SUCCESS(FindRegistryItem(&insertedRegPath, entryHead, currentItemCount, &foundEntry))) {
    //    return;
    //}
    //// only add if the item does not exist in the list
    //if (foundEntry == nullptr) {
    //    InsertTailList(entryHead, entry);
    //    currentItemCount++;
    //}
    InsertTailList(entryHead, entry);
    currentItemCount++;
}

// automatically lock the list
void MtPopItem(__in LIST_ENTRY* entry, __inout int& currentItemCount, FastMutex itemLock)
{
    AutoLock<FastMutex> lock(itemLock); // till now to the end of the function we will have Mutex
                                               // and will be freed on destructor at the end of the function
    // only remove item when list is not empty
    if (currentItemCount > 0)
    {
        RemoveEntryList(entry);
        currentItemCount--;
    }
}

// automatically lock the list
void MtViewItem(LIST_ENTRY* entryHead, ItemType itemType, int currentItemCount, FastMutex itemLock)
{
    AutoLock<FastMutex> lock(itemLock);
    if (currentItemCount > 0) {
        PLIST_ENTRY pEntry = entryHead->Flink;
        if (pEntry != NULL && pEntry != entryHead) {
            int i = 0;
            do {
                switch (itemType) {
                case ItemType::ProgramBlockPath:
                {
                    // info will has the format as a pointer to a DOItem Struct
                    auto info = CONTAINING_RECORD(pEntry, DOItem<ProcessMonitorInfo>, Entry);
                    auto kName = info->Data.ProcessPath;
                    UNICODE_STRING procPath;
                    RtlInitUnicodeString(&procPath, kName);
                    if (NULL != procPath.Buffer) {
                        KdPrint((DO_DRIVER_PREFIX "Process Path in List: %wZ!\r\n", procPath));
                        //RtlFreeUnicodeString(&regPath);
                    }
                    break;
                }
                }
                pEntry = pEntry->Flink;
                i++;
            } while (i < currentItemCount && pEntry != entryHead && pEntry != NULL);
        }
    }
    else {
        KdPrint((DO_DRIVER_PREFIX "Linked List is empty\r\n"));
    }
}

// the caller must acquire lock for it
// result is from `foundEntry`
// require checking returned status before using the foundEntry
NTSTATUS FindItem(__in PUNICODE_STRING pTargetItem, __in LIST_ENTRY* entryHead, __in ItemType itemType, __in int currentItemCount, __out PLIST_ENTRY* foundEntry) {
    NTSTATUS status = STATUS_SUCCESS;
    *foundEntry = nullptr;
    if (currentItemCount > 0) {
        PLIST_ENTRY pEntry = entryHead->Flink;
        if (pEntry != NULL && pEntry != entryHead) {
            int i = 0;
            do {
                switch (itemType) {
                case ItemType::ProgramBlockPath:
                {
                    // info will has the format as a pointer to a DOItem Struct
                    auto info = CONTAINING_RECORD(pEntry, DOItem<ProcessMonitorInfo>, Entry);
                    // if node is not what we are looking for, continue to find in other nodes
                    if (info == NULL || info->Data.ProcessPath == NULL) {
                        status = STATUS_INVALID_ADDRESS;
                        break;
                    }
                    auto kName = info->Data.ProcessPath;
                    UNICODE_STRING procPath;
                    RtlInitUnicodeString(&procPath, kName);
                    if (NULL != procPath.Buffer) {
                        if (RtlCompareUnicodeString(pTargetItem, &procPath, TRUE) == 0) {
                            *foundEntry = pEntry;
                            break;
                        }
                    }
                    break;
                }
                default: {
                    status = STATUS_NOT_FOUND;
                    return status;
                }
                }

                pEntry = pEntry->Flink;
                i++;
            } while (i < currentItemCount && pEntry != entryHead && pEntry != NULL);
        }
    }
    return status;
}