#include "pch.h"
#include "notifyroutine.h"
#include "datastore.h"
#include "drivercommon.h"
#include "linkedlisthelpers.h"

#include "autolock.h"

extern Globals g_Globals;

// can replace this function with helper in linkedlisthelpers.cpp but we still keep it here for a simple tutorial for beginners
void PushProcNotiItem(LIST_ENTRY* entry) {
	// since linked list may be accessed concurrently by multiple threads, protect it with a mutex or a fast mutex
	// lock the fast mutex
	AutoLock<FastMutex> lock(g_Globals.ProcNotiLock);
	if (g_Globals.ProcNotiItemsCount> MAX_PROC_NOTI_COUNT) {
		// too many items, remove oldest one
		auto head = RemoveHeadList(&g_Globals.ProcNotiItemsHead);
		g_Globals.ProcNotiItemsCount--;

		// 1. because the `head` aka Entry is a field of DOItem<T>, instead of freeing the memory of `head`, we need to free entire its instance of DOItem<T>  
		// 2. which template T should we find? 
		// - ProcessExitInfo or ProcessCreateInfo, they both ok
		// - but to store them both in a same linked list, we can use ItemHeader as template T
		// - the above approach is only for Cpp coding style. In C coding style, there's no such thing called inheritance -> we have to specify which type of template we want to get
		// get the address of an instance of DOItem<ItemHeader> which contains the removed `head`
		auto item = CONTAINING_RECORD(head, DOItem<ItemHeader>, Entry);
		// free the instance of DOItem<ItemHeader>
		ExFreePool(item);
	}
	// add the entry to the end of global list
	InsertTailList(&g_Globals.ProcNotiItemsHead, entry);
	g_Globals.ProcNotiItemsCount++;

	// when out of scope, AutoLock call its destructor, then the fast mutex would be unlock
}

void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo) {
	UNREFERENCED_PARAMETER(Process);
	NTSTATUS status = STATUS_SUCCESS;
	bool isBlock = false;
	// process create
	if (CreateInfo) {
		if (g_Globals.ProgItemsCount > 0) {
			if (CreateInfo->FileOpenNameAvailable && CreateInfo->ImageFileName)
			{
				//KdPrint((DO_DRIVER_PREFIX "ImageFilePath: %wZ\n", CreateInfo->ImageFileName));

				AutoLock<FastMutex> lock(g_Globals.ProgLock);

				PLIST_ENTRY foundEntry = nullptr;
				UNICODE_STRING imgFileName;
				RtlInitUnicodeString(&imgFileName, CreateInfo->ImageFileName->Buffer);
				status = FindItem(&imgFileName, &g_Globals.ProgItemsHead, ItemType::ProgramBlockPath, g_Globals.ProgItemsCount, &foundEntry);
				if (!NT_SUCCESS(status)) {
					// use status for logging
					return;
				}

				// only block if the item does exist in the list
				if (foundEntry != nullptr) {
					isBlock = true;
					CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;
					KdPrint((DO_DRIVER_PREFIX "Program not allowed to Execute: %wZ\n", CreateInfo->ImageFileName));
				}
			}
		}


		// allocate the total size of allocation for DOItem<T>
		USHORT allocSize = sizeof(DOItem<ProcessCreateInfo>);
		USHORT commandLineSize = 0;

		if (CreateInfo->CommandLine) {
			// set the size of cmd
			commandLineSize = CreateInfo->CommandLine->Length;
			// if there's cmd, the total size also includes the length of command line
			// why? we are going to insert the cmd to the end of DOItem<T> -> we have to allocate the memory for both of them
			allocSize += commandLineSize;
		}
		// get an obj of FullIteam with template as ProcessCreateInfo
		auto info = (DOItem<ProcessCreateInfo>*)ExAllocatePoolWithTag(PagedPool,
			allocSize, DRIVER_TAG);
		if (info == nullptr) {
			KdPrint(("failed allocation\n"));
			return;
		}
		// prepare data for an item of linked list
		auto& item = info->Data;
		// set current time (UTC)
		KeQuerySystemTimePrecise(&item.Time);
		// set type
		item.Type = ItemType::ProcessCreate;
		// set status
		if (isBlock) {
			item.Status = ItemStatus::Blocked;
		}
		else {
			item.Status = ItemStatus::Success;
		}
		// set process id
		item.ProcessId = HandleToULong(ProcessId);
		// set parent process id
		item.ParentProcessId = HandleToULong(CreateInfo->ParentProcessId);
		// set size(item) = size(base_structure) + size(cmd)
		item.Size = sizeof(ProcessCreateInfo) + commandLineSize;

		if (commandLineSize > 0) {
			// copy cmd to the address of the end of the base structure
			::memcpy((UCHAR*)&item + sizeof(item), CreateInfo->CommandLine->Buffer,
				commandLineSize);
			item.CommandLineLength = commandLineSize / sizeof(WCHAR); // length in WCHARs
			// set the offset as the size of the base structure
			item.CommandLineOffset = sizeof(item);
		}
		else {
			item.CommandLineLength = 0;
		}
		// push data to linked list
		PushProcNotiItem(&info->Entry);
	}
	else {
		// process exit
		// get an obj of FullIteam with template as ProcessExitInfo
		auto info = (DOItem<ProcessExitInfo>*)ExAllocatePoolWithTag(PagedPool,
			sizeof(DOItem<ProcessExitInfo>), DRIVER_TAG);
		if (info == nullptr) {
			KdPrint(("failed allocation\n"));
			return;
		}
		// prepare data for an item of linked list
		auto& item = info->Data;
		// set current time (UTC)
		KeQuerySystemTimePrecise(&item.Time);
		// set type
		item.Type = ItemType::ProcessExit;
		// set process id
		item.ProcessId = HandleToULong(ProcessId);
		// set size
		item.Size = sizeof(ProcessExitInfo);

		// push data to linked list
		// our global linked list will contain only entry of DOItem<ProcessExitInfo>
		PushProcNotiItem(&info->Entry);
	}
}