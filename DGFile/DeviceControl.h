
#pragma once

#define DEVICE_CONTROL_TAG 'OCED' // DECO

VOID PrintUnknownDeviceIoctlCode(__in ULONG IoctlCode);

NTSTATUS
GlobalDeviceControl(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

VOID DokanPopulateDiskGeometry(__in PDISK_GEOMETRY diskGeometry);

NTSTATUS
DiskDeviceControl(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchDeviceControl(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
