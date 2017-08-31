
#pragma once

#define MOUNT_TAG 'NUOM' // MOUN

NTSTATUS IsMountPointDriveLetter(__in PUNICODE_STRING mountPoint);

VOID DokanDeleteMountPoint(__in PDokanDCB Dcb);

VOID DokanCreateMountPoint(__in PDokanDCB Dcb);

NTSTATUS
DokanGetMountPointList(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in PDOKAN_GLOBAL dokanGlobal);

PMOUNT_ENTRY
FindMountEntry(__in PDOKAN_GLOBAL dokanGlobal,
	__out PDOKAN_CONTROL DokanControl);

NTSTATUS
DokanSendIoContlToMountManager(__in ULONG IoControlCode, __in PVOID InputBuffer,
	__in ULONG Length, __out PVOID OutputBuffer,
	__in ULONG OutputLength);

VOID DokanUnmount(__in PDokanDCB Dcb);

NTSTATUS DokanMountVolume(__in PDEVICE_OBJECT DiskDevice, __in PIRP Irp);

PMOUNT_ENTRY
InsertMountEntry(PDOKAN_GLOBAL dokanGlobal, PDOKAN_CONTROL DokanControl);

VOID RemoveMountEntry(PDOKAN_GLOBAL dokanGlobal, PMOUNT_ENTRY MountEntry);

VOID DokanCreateMountPointSysProc(__in PDokanDCB Dcb);

VOID DokanDeleteMountPointSysProc(__in PDokanDCB Dcb);
