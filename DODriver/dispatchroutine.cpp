#include "pch.h"
#include "dispatchroutine.h"

#include "drivercommon.h"
#include "autolock.h"
#include "datastore.h"
#include "csstring.h"
#include "linkedlisthelpers.h"

extern Globals g_Globals;

// irp helper for avoid duplicate code
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	// set status of this request
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	// propagate the IRP back to its creator (I/O Manager, Plug & Play Manager or Power Manager)
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS
DOCreateAndClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	return CompleteIrp(Irp, STATUS_SUCCESS);
}

NTSTATUS
DODirectIOWrite(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;

	auto count = 0;

	if (len == 0) {
		return CompleteIrp(Irp, STATUS_INVALID_BUFFER_SIZE, count);
	}

	// check if we can access Irp->MdlAddress
	NT_ASSERT(Irp->MdlAddress);

	auto buffer = (UCHAR*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!buffer) {
		return CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, count);
	}
	else {
		// acquire mutex without lock template: required C++ 17
		AutoLock lock(g_Globals.ProcNotiLock);

		// let the client poll the driver for information
		// fill the buffer with as many events as possible, until out of buffer or no more events in the queue (which is our linked list)
		while (true) {
			// stop if the queue is empty
			if (IsListEmpty(&g_Globals.ProcNotiItemsHead)) // can also check g_Globals.ItemCount
				break;
			auto entry = RemoveHeadList(&g_Globals.ProcNotiItemsHead);
			auto info = CONTAINING_RECORD(entry, DOItem<ItemHeader>, Entry);
			auto size = info->Data.Size;
			if (len < size) {
				// user's buffer is full, insert item back
				InsertHeadList(&g_Globals.ProcNotiItemsHead, entry);
				// stop when buffer is exhausted (full)
				break;
			}
			g_Globals.ProcNotiItemsCount--;
			::memcpy(buffer, &info->Data, size);
			len -= size;
			buffer += size;
			count += size;
			// free data after copy
			ExFreePool(info);
		}
	}

	return CompleteIrp(Irp, STATUS_SUCCESS, count);
}


NTSTATUS
DODeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	// get our IO_STACK_LOCATION
	auto stack = IoGetCurrentIrpStackLocation(Irp); // IO_STACK_LOCATION*
	auto status = STATUS_SUCCESS;

	static const WCHAR machinePath[] = L"\\REGISTRY\\MACHINE\\";
	static const WCHAR userPath[] = L"\\REGISTRY\\USER\\";

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_DO_PROGRAM_BLOCK:
	{
		auto inputBufferSize = stack->Parameters.DeviceIoControl.InputBufferLength;
		auto inputBuffer = (WCHAR*)Irp->AssociatedIrp.SystemBuffer;

		if (inputBufferSize == 0 || inputBuffer == nullptr)
		{
			KdPrint((DO_DRIVER_PREFIX "The Process path passed is not Correct.\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		// check if process path valid or not
		if (::wcslen(inputBuffer) < 3) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		// check if our path starts with `X:\`
		if (inputBuffer[1] != L':' || inputBuffer[2] != L'\\') {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		//KdPrint((DRIVER_PREFIX "The Process path to Protect is: %ws", inputBuffer));

		auto size = sizeof(DOItem<ProcessMonitorInfo>);
		auto info = (DOItem<ProcessMonitorInfo>*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);
		if (info == nullptr)
		{
			KdPrint((DO_DRIVER_PREFIX "Failed to Allocate Memory.\n"));
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RtlZeroMemory(info, size);

		do {
			auto& item = info->Data;
			//auto RegKeyLength = inputBufferSize / sizeof(WCHAR);
			//RtlCopyMemory(item.ProcessPath, inputBuffer, inputBufferSize);
			csstring symLinkFileName(L"\\??\\");
			symLinkFileName.Append(inputBuffer, inputBufferSize + 1);
			RtlCopyMemory(item.ProcessPath, symLinkFileName.Get(), symLinkFileName.Length());

			item.Type = ItemType::ProgramBlockPath;

			AutoLock<FastMutex> lock(g_Globals.ProgLock);

			UNICODE_STRING insertedProcPath;
			RtlInitUnicodeString(&insertedProcPath, item.ProcessPath);
			PLIST_ENTRY foundEntry = nullptr;
			status = FindItem(&insertedProcPath, &g_Globals.ProgItemsHead, item.Type, g_Globals.ProgItemsCount, &foundEntry);
			if (!NT_SUCCESS(status)) {
				break;
			}
			// only add if the item does not exist in the list
			if (foundEntry == nullptr) {
				PushItem(&(info->Entry), &g_Globals.ProgItemsHead, g_Globals.ProgItemsCount, MAX_PROG_BLOCK_COUNT);
			}
			KdPrint((DO_DRIVER_PREFIX "Block process path %ws", inputBuffer));
		} while (false);

		MtViewItem(&g_Globals.ProgItemsHead, ItemType::ProgramBlockPath, g_Globals.ProgItemsCount, g_Globals.ProgLock);

		break;
	}
	case IOCTL_DO_PROGRAM_UNBLOCK:
	{
		auto inputBufferSize = stack->Parameters.DeviceIoControl.InputBufferLength;
		auto inputBuffer = (WCHAR*)Irp->AssociatedIrp.SystemBuffer;

		if (inputBufferSize == 0 || inputBuffer == nullptr)
		{
			KdPrint((DO_DRIVER_PREFIX "The Process path passed is not Correct.\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		KdPrint((DO_DRIVER_PREFIX "The Process path to Unblock is: %ws", inputBuffer));

		// check if process path valid or not
		if (::wcslen(inputBuffer) < 3) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		// check if our path starts with `X:\`
		if (inputBuffer[1] != L':' || inputBuffer[2] != L'\\') {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		// use csstring to easier comparation
		csstring symLinkFileName(L"\\??\\");
		symLinkFileName.Append(inputBuffer, inputBufferSize + 1);
		UNICODE_STRING symLinkFileNameUnicode;

		do {
			// find and remove the process path from the linked list
			AutoLock<FastMutex> lock(g_Globals.ProgLock);
			// we already acquire the mutex -> safely call FindRegistryItem
			PLIST_ENTRY foundEntry = nullptr;
			if (!NT_SUCCESS(FindItem(symLinkFileName.GetUnicodeString(&symLinkFileNameUnicode), &g_Globals.ProgItemsHead, ItemType::ProgramBlockPath, g_Globals.ProgItemsCount, &foundEntry))) {
				// temporarily consider this as a normal case (we don't remove anything)
				break;
			}
			// only remove if the item does exist in the list
			if (foundEntry == nullptr) {
				// temporarily consider this as a normal case (we don't remove anything)
				//status = STATUS_NOT_FOUND;
				break;
			}

			// if result is TRUE, linked list is empty -> update the count track
			if (RemoveEntryList(foundEntry)) {
				g_Globals.ProgItemsCount = 0;
			}
			else {
				g_Globals.ProgItemsCount--;
			}
			ExFreePool(CONTAINING_RECORD(foundEntry, DOItem<ProcessMonitorInfo>, Entry));
			break;
		} while (false);
		MtViewItem(&g_Globals.ProgItemsHead, ItemType::ProgramBlockPath, g_Globals.ProgItemsCount, g_Globals.ProgLock);
		break;
	}
	default: {
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	}
	return CompleteIrp(Irp, status);
}