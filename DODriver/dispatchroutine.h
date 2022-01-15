#pragma once

NTSTATUS
DOCreateAndClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

NTSTATUS
DODirectIOWrite(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

NTSTATUS
DODeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

// helpers
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);

