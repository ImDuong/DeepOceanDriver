#pragma once


NTSTATUS
SysMonRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

// helpers
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);

NTSTATUS
S1mpleCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);