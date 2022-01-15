#include "pch.h"
#include "dispatchroutine.h"

#include "drivercommon.h"
#include "autolock.h"
#include "datastore.h"

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
		AutoLock lock(g_Globals.NotiLock);

		// let the client poll the driver for information
		// fill the buffer with as many events as possible, until out of buffer or no more events in the queue (which is our linked list)
		while (true) {
			// stop if the queue is empty
			if (IsListEmpty(&g_Globals.ItemsHead)) // can also check g_Globals.ItemCount
				break;
			auto entry = RemoveHeadList(&g_Globals.ItemsHead);
			auto info = CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry);
			auto size = info->Data.Size;
			if (len < size) {
				// user's buffer is full, insert item back
				InsertHeadList(&g_Globals.ItemsHead, entry);
				// stop when buffer is exhausted (full)
				break;
			}
			g_Globals.ItemCount--;
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