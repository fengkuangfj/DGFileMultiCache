
#pragma once

#define DEVICE_TAG 'IVED' // DEVI

VOID DokanDeleteDeviceObject(__in PDokanDCB Dcb);

NTSTATUS
DokanRegisterMountedDeviceInterface(__in PDEVICE_OBJECT DeviceObject,
	__in PDokanDCB Dcb);

NTSTATUS
DokanRegisterDeviceInterface(__in PDRIVER_OBJECT DriverObject,
	__in PDEVICE_OBJECT DeviceObject,
	__in PDokanDCB Dcb);

NTSTATUS
DokanCreateGlobalDiskDevice(__in PDRIVER_OBJECT DriverObject,
	__out PDOKAN_GLOBAL *DokanGlobal);

NTSTATUS
DokanCreateDiskDevice(__in PDRIVER_OBJECT DriverObject, __in ULONG MountId,
	__in PWCHAR MountPoint, __in PWCHAR UNCName,
	__in PWCHAR BaseGuid, __in PDOKAN_GLOBAL DokanGlobal,
	__in DEVICE_TYPE DeviceType,
	__in ULONG DeviceCharacteristics,
	__in BOOLEAN MountGlobally, __in BOOLEAN UseMountManager,
	__out PDokanDCB *Dcb);

NTSTATUS
QueryDeviceRelations(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
