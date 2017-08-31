
#pragma once

#define DEVICE_TAG 'IVED' // DEVI




NTSTATUS IsMountPointDriveLetter(__in PUNICODE_STRING mountPoint);

NTSTATUS
DokanSendIoContlToMountManager(__in ULONG IoControlCode, __in PVOID InputBuffer,
	__in ULONG Length, __out PVOID OutputBuffer,
	__in ULONG OutputLength);

NTSTATUS
DokanSendVolumeArrivalNotification(PUNICODE_STRING DeviceName);

NTSTATUS
DokanSendVolumeDeletePoints(__in PUNICODE_STRING MountPoint,
	__in PUNICODE_STRING DeviceName);

NTSTATUS
DokanSendVolumeCreatePoint(__in PUNICODE_STRING DeviceName,
	__in PUNICODE_STRING MountPoint);

NTSTATUS
DokanRegisterMountedDeviceInterface(__in PDEVICE_OBJECT DeviceObject,
	__in PDokanDCB Dcb);

NTSTATUS
DokanRegisterDeviceInterface(__in PDRIVER_OBJECT DriverObject,
	__in PDEVICE_OBJECT DeviceObject,
	__in PDokanDCB Dcb);

VOID DokanInitIrpList(__in PIRP_LIST IrpList);

PMOUNT_ENTRY
InsertMountEntry(PDOKAN_GLOBAL dokanGlobal, PDOKAN_CONTROL DokanControl);

VOID RemoveMountEntry(PDOKAN_GLOBAL dokanGlobal, PMOUNT_ENTRY MountEntry);

PMOUNT_ENTRY
FindMountEntry(__in PDOKAN_GLOBAL dokanGlobal,
	__out PDOKAN_CONTROL DokanControl);

NTSTATUS
DokanGetMountPointList(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in PDOKAN_GLOBAL dokanGlobal);

NTSTATUS
DokanCreateGlobalDiskDevice(__in PDRIVER_OBJECT DriverObject,
	__out PDOKAN_GLOBAL *DokanGlobal);

PUNICODE_STRING
DokanAllocateUnicodeString(__in PCWSTR String);

VOID FreeUnicodeString(PUNICODE_STRING UnicodeString);

static VOID FreeDcbNames(__in PDokanDCB Dcb);

VOID DokanRegisterUncProvider(__in PDokanDCB Dcb);

NTSTATUS DokanRegisterUncProviderSystem(PDokanDCB dcb);

//KSTART_ROUTINE DokanDeregisterUncProvider;
VOID DokanDeregisterUncProvider(__in PDokanDCB Dcb);

VOID DokanCreateMountPointSysProc(__in PDokanDCB Dcb);

VOID DokanCreateMountPoint(__in PDokanDCB Dcb);

//KSTART_ROUTINE DokanDeleteMountPointSysProc;
VOID DokanDeleteMountPointSysProc(__in PDokanDCB Dcb);

VOID DokanDeleteMountPoint(__in PDokanDCB Dcb);


NTSTATUS
DokanCreateDiskDevice(__in PDRIVER_OBJECT DriverObject, __in ULONG MountId,
	__in PWCHAR MountPoint, __in PWCHAR UNCName,
	__in PWCHAR BaseGuid, __in PDOKAN_GLOBAL DokanGlobal,
	__in DEVICE_TYPE DeviceType,
	__in ULONG DeviceCharacteristics,
	__in BOOLEAN MountGlobally, __in BOOLEAN UseMountManager,
	__out PDokanDCB *Dcb);

	VOID DokanDeleteDeviceObject(__in PDokanDCB Dcb);
