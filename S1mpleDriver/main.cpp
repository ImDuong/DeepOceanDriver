#include "pch.h"
#include "dispatchroutine.h"
#include "s1mplecommon.h"

#include "sysmon.h"
#include "notifyroutine.h"
#include "operationcallback.h"
#include "registrynotifyroutine.h"

UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\S1mpleDevice");
UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\S1mpleDevice");

Globals g_Globals;

void S1mpleUnload(_In_ PDRIVER_OBJECT DriverObject) {
	// undo anything done in DriverEntry in reverse order with respect to the init order in driver entry
	
	// unregister registry notification
	CmUnRegisterCallback(g_Globals.RegCookie);

	// unregister image load notification 
	PsRemoveLoadImageNotifyRoutine(OnImageLoadNotify);

	// unregister thread notification 
	PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);

	// unregister process notification 
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);

	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);

	// unregister callback
	ObUnRegisterCallbacks(g_Globals.RegHandle);

	// free linked list
	// because we already unregister process, thread, image load notification -> no need to acquire mutex for freeing linked list
	while (!IsListEmpty(&g_Globals.ItemsHead)) {
		auto entry = RemoveHeadList(&g_Globals.ItemsHead);
		// thanks to the power of inheritance in Cpp, we can access the base structure with an abstract template
		ExFreePool(CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry));
	}

	// free linked list for registry

	KdPrint(("S1mple Driver unload successfully\n"));
}

extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	// init global variables
	g_Globals.Init();

	OB_OPERATION_REGISTRATION operations[] = {
		{
			PsProcessType,		// object type: process
			OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,
			OnPreOpenProcess, nullptr	// pre, post operation
		}
	};
	OB_CALLBACK_REGISTRATION reg = {
		OB_FLT_REGISTRATION_VERSION,
		1,				// operation count
		RTL_CONSTANT_STRING(L"12345.6969"),		// altitude: higher altitude, the earlier the driver is invoked
		nullptr,		// context
		operations
	};


	PDEVICE_OBJECT DeviceObject = nullptr;
	auto status = STATUS_SUCCESS;
	auto symLinkCreated = false;
	auto processCallBackCreated = false;
	auto threadCallBackCreated = false;
	auto imageLoadCallBackCreated = false;


	// use one loop technique for avoiding the use of goto
	do {
		status = ObRegisterCallbacks(&reg, &g_Globals.RegHandle);
		if (!NT_SUCCESS(status)) {
			KdPrint(("failed to register callbacks (status=%08X)\n", status));
			break;
		}


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

		// register for thread notifications
		status = PsSetCreateThreadNotifyRoutine(OnThreadNotify);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failed to register thread callback (0x%08X)\n", status));
			break;
		}
		threadCallBackCreated = true;

		status = PsSetLoadImageNotifyRoutine(OnImageLoadNotify);
		if (!NT_SUCCESS(status)) {
			KdPrint(("failed to set image load callback (status=%08X)\n", status));
			break;
		}
		imageLoadCallBackCreated = true;

		UNICODE_STRING altitude = RTL_CONSTANT_STRING(L"7657.124");
		status = CmRegisterCallbackEx(OnRegistryNotify, &altitude, DriverObject, nullptr, &g_Globals.RegCookie, nullptr);
		if (!NT_SUCCESS(status)) {
			KdPrint(("failed to set registry callback (status=%08X)\n", status));
			break;
		}

	} while (false);

	if (!NT_SUCCESS(status)) {
		if (imageLoadCallBackCreated) {
			PsRemoveLoadImageNotifyRoutine(OnImageLoadNotify);
		}

		if (threadCallBackCreated) {
			PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);
		}

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

		if (g_Globals.RegHandle) {
			ObUnRegisterCallbacks(g_Globals.RegHandle);
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
	DriverObject->DriverUnload = S1mpleUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = S1mpleCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = S1mpleCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = S1mpleDeviceControl;
	//DriverObject->MajorFunction[IRP_MJ_READ] = S1mpleRead;
	DriverObject->MajorFunction[IRP_MJ_READ] = SysMonRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = S1mpleWrite;

	KdPrint(("S1mple Driver init successfully\n"));

	return STATUS_SUCCESS;
}