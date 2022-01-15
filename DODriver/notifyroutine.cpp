#include "pch.h"
#include "notifyroutine.h"
#include "sysmon.h"
#include "s1mplecommon.h"

#include "autolock.h"

extern Globals g_Globals;

void PushItem(LIST_ENTRY* entry) {
	// since linked list may be accessed concurrently by multiple threads, protect it with a mutex or a fast mutex
	// lock the fast mutex
	AutoLock<FastMutex> lock(g_Globals.NotiLock);
	if (g_Globals.ItemCount > MAX_ITEM_COUNT) {
		// too many items, remove oldest one
		auto head = RemoveHeadList(&g_Globals.ItemsHead);
		g_Globals.ItemCount--;

		// 1. because the `head` aka Entry is a field of FullItem<T>, instead of freeing the memory of `head`, we need to free entire its instance of FullItem<T>  
		// 2. which template T should we find? 
		// - ProcessExitInfo or ProcessCreateInfo, they both ok
		// - but to store them both in a same linked list, we can use ItemHeader as template T
		// - the above approach is only for Cpp coding style. In C coding style, there's no such thing called inheritance -> we have to specify which type of template we want to get
		// get the address of an instance of FullItem<ItemHeader> which contains the removed `head`
		auto item = CONTAINING_RECORD(head, FullItem<ItemHeader>, Entry);
		// free the instance of FullItem<ItemHeader>
		ExFreePool(item);
	}
	// add the entry to the end of global list
	InsertTailList(&g_Globals.ItemsHead, entry);
	g_Globals.ItemCount++;

	// when out of scope, AutoLock call its destructor, then the fast mutex would be unlock
}

void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo) {

	UNREFERENCED_PARAMETER(Process);
	if (CreateInfo) {
		// process create

		// allocate the total size of allocation for FullItem<T>
		USHORT allocSize = sizeof(FullItem<ProcessCreateInfo>);
		USHORT commandLineSize = 0;

		if (CreateInfo->CommandLine) {
			// set the size of cmd
			commandLineSize = CreateInfo->CommandLine->Length;
			// if there's cmd, the total size also includes the length of command line
			// why? we are going to insert the cmd to the end of FullItem<T> -> we have to allocate the memory for both of them
			allocSize += commandLineSize;
		}
		// get an obj of FullIteam with template as ProcessCreateInfo
		auto info = (FullItem<ProcessCreateInfo>*)ExAllocatePoolWithTag(PagedPool,
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
		PushItem(&info->Entry);
	}
	else {
		// process exit
		// get an obj of FullIteam with template as ProcessExitInfo
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePoolWithTag(PagedPool,
			sizeof(FullItem<ProcessExitInfo>), DRIVER_TAG);
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
		// our global linked list will contain only entry of FullItem<ProcessExitInfo>
		PushItem(&info->Entry);
	}
}