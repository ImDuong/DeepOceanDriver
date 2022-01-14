#pragma once


NTSTATUS
S1mpleCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

NTSTATUS
S1mpleDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

NTSTATUS
S1mpleRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

NTSTATUS
SysMonRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

NTSTATUS
S1mpleWrite(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

NTSTATUS
S1mpleDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

// helpers
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);