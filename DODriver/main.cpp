#include "pch.h"
#include "dispatchroutine.h"
#include "drivercommon.h"

#include "datastore.h"
#include "notifyroutine.h"

Globals g_Globals;

UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\DeepOceanDevice");
UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\DeepOceanDevice");

void DOUnload(_In_ PDRIVER_OBJECT DriverObject) {
	// undo anything done in DriverEntry in reverse order with respect to the init order in driver entry
	
	// unregister process notification 
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);

	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);

	// free process noti linked list
	while (!IsListEmpty(&g_Globals.ProcNotiItemsHead)) {
		auto entry = RemoveHeadList(&g_Globals.ProcNotiItemsHead);
		// thanks to the power of inheritance in Cpp, we can access the base structure with an abstract template
		ExFreePool(CONTAINING_RECORD(entry, DOItem<ItemHeader>, Entry));
	}

	// free blocking process linked list
	while (!IsListEmpty(&g_Globals.ProgItemsHead)) {
		auto entry = RemoveHeadList(&g_Globals.ProgItemsHead);
		ExFreePool(CONTAINING_RECORD(entry, DOItem<ProcessMonitorInfo>, Entry));
	}

	// free linked list for registry
	KdPrint(("Deep Ocean Driver unload successfully\n"));
}

extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	// init global variables
	g_Globals.Init();

	PDEVICE_OBJECT DeviceObject = nullptr;
	auto status = STATUS_SUCCESS;
	auto symLinkCreated = false;
	auto processCallBackCreated = false;


	// use one loop technique for avoiding the use of goto
	do {
		// create device obj
		status = IoCreateDevice(
			_In_ DriverObject,					// driver obj
			_In_ 0,								// extra bytes for extra structure
			_In_opt_ & devName,					// optional: device name
			_In_ FILE_DEVICE_UNKNOWN,			// device type: should use FILE_DEVICE_UNKNOWN for software driver
			_In_ 0,								// characteristic flags
			_In_ FALSE,							// TRUE for allowing multiple handle to device object
			_Outptr_ & DeviceObject				// our target
		);

		if (!NT_SUCCESS(status)) {
			KdPrint(("Create device object unsuccessfully: 0x%08X\n", status));
			break;
		}

		// setup direct I/O (this flag only works for IRP_MJ_READ and IRP_MJ_WRITE)
		DeviceObject->Flags |= DO_DIRECT_IO;

		// create symbolic link (allow user mode can access device obj via symlink)
		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Create symbolic link for device object unsuccessfully: 0x%08X\n", status));
			break;
		}
		symLinkCreated = true;

		// register for process notifications
		status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to register process callback (0x%08X)\n", status));
			break;
		}
		processCallBackCreated = true;
	} while (false);

	if (!NT_SUCCESS(status)) {
		if (processCallBackCreated) {
			PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
		}

		// if symlink has been constructed, delete it to avoid memory leak
		if (symLinkCreated) {
			IoDeleteSymbolicLink(&symLink);
		}

		// if device obj has been constructed, delete it to avoid memory leak
		if (DeviceObject) {
			IoDeleteDevice(DeviceObject);
		}
	}

	// get Windows OS version: major, minor, build number
	RTL_OSVERSIONINFOW osVersion;
	status = RtlGetVersion(&osVersion);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Get OS version unsuccessfully: 0x%08X\n", status));
	}
	else {
		KdPrint(("OS major version: %u\n", osVersion.dwMajorVersion));
		KdPrint(("OS minor version: %u\n", osVersion.dwMinorVersion));
		KdPrint(("OS build number: %u\n", osVersion.dwBuildNumber));
	}

	// attach operations
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DOCreateAndClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DOCreateAndClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DODeviceControl;
	DriverObject->DriverUnload = DOUnload;

	DriverObject->MajorFunction[IRP_MJ_READ] = DODirectIOWrite;
	KdPrint(("Deep Ocean Driver init successfully\n"));
	return STATUS_SUCCESS;
}