#include "pch.h"
#include "dispatchroutine.h"

#include "s1mplecommon.h"
#include "autolock.h"
#include "sysmon.h"
#include "operationcallback.h"

// globals
long long g_TotalByteRead = 0, g_TotalByteWritten = 0;

extern Globals g_Globals;

void PushRegItem(LIST_ENTRY* entry)
{
	AutoLock<FastMutex> lock(g_Globals.RegProtLock); // till now to the end of the function we will have Mutex
											   // and will be freed on destructor at the end of the function
	if (g_Globals.RegItemCount > MaxRegKeyCount)
	{
		// too many items, remove oldest one
		auto head = RemoveHeadList(&g_Globals.RegItemsHead);
		g_Globals.RegItemCount--;
		ExFreePool(CONTAINING_RECORD(head, FullItem<RegistryKeyProtectInfo*>, Entry));
	}

	InsertTailList(&g_Globals.RegItemsHead, entry);
	g_Globals.RegItemCount++;
}

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	// set status of this request
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	// propagate the IRP back to its creator (I/O Manager, Plug & Play Manager or Power Manager)
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS
S1mpleCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	return CompleteIrp(Irp, STATUS_SUCCESS);
	//// set status of this request
	//Irp->IoStatus.Status = STATUS_SUCCESS;
	//Irp->IoStatus.Information = 0;
	//// propagate the IRP back to its creator (I/O Manager, Plug & Play Manager or Power Manager)
	//IoCompleteRequest(Irp, IO_NO_INCREMENT);
	//return STATUS_SUCCESS;
}

NTSTATUS
S1mpleRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	if (len == 0) {
		return CompleteIrp(Irp, STATUS_INVALID_BUFFER_SIZE);
	}
	auto buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!buffer) {
		return CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES);
	}

	// ZERO BUFFER MODE: 
	// zero all buffer read from client
	memset(buffer, 0, len);

	// synchronize access to global var
	InterlockedAdd64(&g_TotalByteRead, len);

	// fill Irp->IoStatus.Information with length of the buffer because client needs to know the number of bytes consumed in operation (via arguments in ReadFile API)
	return CompleteIrp(Irp, STATUS_SUCCESS, len);

}

NTSTATUS
S1mpleWrite(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	if (len == 0) {
		return CompleteIrp(Irp, STATUS_INVALID_BUFFER_SIZE);
	}

	// synchronize access to global var
	InterlockedAdd64(&g_TotalByteWritten, len);
	return CompleteIrp(Irp, STATUS_SUCCESS, len);

}

NTSTATUS
S1mpleDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	// get our IO_STACK_LOCATION
	auto stack = IoGetCurrentIrpStackLocation(Irp); // IO_STACK_LOCATION*
	auto status = STATUS_SUCCESS;
	auto len = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_S1MPLE_SET_PRIORITY: {
		// check if buffer is big enough
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		auto data = (ThreadData*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
		if (data == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		if (data->Priority < 1 || data->Priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		PETHREAD Thread;
		// handle is 64 bit on 64 bit system
		// but thread ID provided by client is always 32 bit -> need to cast via ULongToHandle
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &Thread);
		if (!NT_SUCCESS(status)) {
			break;
		}
		// after creating new reference to target thread, we are safely to change its priority
		// even the thread terminates, the thread obj still exists because we just increase its reference count via PsLookupThreadByThreadId API
		KeSetPriorityThread((PKTHREAD)Thread, data->Priority);

		// dereference thread obj
		ObDereferenceObject(Thread);
		KdPrint(("Thread priority change from %d to %d successfully\n", data->ThreadId, data->Priority));
		break;
	}
	case IOCTL_S1MPLE_GET_ZERO_STATS: {
		// check if buffer is big enoughe
		if (stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ZeroStats)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		// fullfill data for sending to client
		
		// access MDL address: IOCTL code using METHOD_OUT_DIRECT or METHOD_IN_DIRECT
		auto buffer = (ZeroStats*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
		if (!buffer) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		buffer->TotalRead = g_TotalByteRead;
		buffer->TotalWritten = g_TotalByteWritten;

		// write data to system buffer: IOCTL code using METHOD_BUFFER
		/*auto buffer = (ZeroStats*)Irp->AssociatedIrp.SystemBuffer;
		buffer->TotalRead = g_TotalByteRead;
		buffer->TotalWritten = g_TotalByteWritten;*/

		return CompleteIrp(Irp, status, sizeof(ZeroStats));
		break;
	}
	case IOCTL_S1MPLE_PROTECT_PROCESS_BY_PID:
	{
		auto size = stack->Parameters.DeviceIoControl.InputBufferLength;
		if (size % sizeof(ULONG) != 0) {
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		auto data = (ULONG*)Irp->AssociatedIrp.SystemBuffer;

		AutoLock locker(g_Globals.ProcProtLock);

		for (int i = 0; i < size / sizeof(ULONG); i++) {
			auto pid = data[i];
			if (pid == 0) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (FindProcess(pid))
				continue;

			if (g_Globals.PidsCount == MaxPids) {
				status = STATUS_TOO_MANY_CONTEXT_IDS;
				break;
			}

			if (!AddProcess(pid)) {
				status = STATUS_UNSUCCESSFUL;
				break;
			}

			len += sizeof(ULONG);
		}

		break;
	}

	case IOCTL_S1MPLE_UNPROTECT_PROCESS_BY_PID:
	{
		auto size = stack->Parameters.DeviceIoControl.InputBufferLength;
		// check if buffer is a multiple of four bytes (PIDs)
		if (size % sizeof(ULONG) != 0) {
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		auto data = (ULONG*)Irp->AssociatedIrp.SystemBuffer;

		AutoLock locker(g_Globals.ProcProtLock);

		for (int i = 0; i < size / sizeof(ULONG); i++) {
			auto pid = data[i];
			if (pid == 0) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if (!RemoveProcess(pid))
				continue;

			len += sizeof(ULONG);

			if (g_Globals.PidsCount == 0)
				break;
		}

		break;
	}

	case IOCTL_S1MPLE_CLEAR_PROTECT_PROCESS:
	{
		AutoLock locker(g_Globals.ProcProtLock);
		::memset(&g_Globals.Pids, 0, sizeof(g_Globals.Pids));
		g_Globals.PidsCount = 0;
		break;
	}
	case IOCTL_S1MPLE_REGKEY_PROTECT_ADD:
	{
		auto inputBufferSize = stack->Parameters.DeviceIoControl.InputBufferLength;
		auto inputBuffer = (WCHAR*)Irp->AssociatedIrp.SystemBuffer;

		if (inputBufferSize == 0 || inputBuffer == nullptr)
		{
			KdPrint(( "The Registry Key passed is not Correct.\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		KdPrint(("The Registry Path to Protect is: %ws", inputBuffer));

		auto size = sizeof(FullItem<RegistryKeyProtectInfo>);
		auto info = (FullItem<RegistryKeyProtectInfo>*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);
		if (info == nullptr)
		{
			KdPrint(( "Failed to Allocate Memory.\n"));
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RtlZeroMemory(info, size);

		auto& item = info->Data;
		//auto RegKeyLength = inputBufferSize / sizeof(WCHAR);
		RtlCopyMemory(item.KeyName, inputBuffer, inputBufferSize);
		PushRegItem(&info->Entry);
		break;
	}


	default: {
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	}
	return CompleteIrp(Irp, status);
}