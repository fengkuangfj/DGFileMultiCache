
#include "Stdafx.h"
#include "Volume.h"

NTSTATUS
DokanSendVolumeArrivalNotification(PUNICODE_STRING DeviceName) {
	NTSTATUS status;
	PMOUNTMGR_TARGET_NAME targetName;
	ULONG length;

	DDbgPrint("=> DokanSendVolumeArrivalNotification\n");

	length = sizeof(MOUNTMGR_TARGET_NAME) + DeviceName->Length - 1;
	targetName = (PMOUNTMGR_TARGET_NAME)ExAllocatePool(length);

	if (targetName == NULL) {
		DDbgPrint("  can't allocate MOUNTMGR_TARGET_NAME\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(targetName, length);

	targetName->DeviceNameLength = DeviceName->Length;
	RtlCopyMemory(targetName->DeviceName, DeviceName->Buffer, DeviceName->Length);

	status = DokanSendIoContlToMountManager(
		IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION, targetName, length, NULL, 0);

	if (NT_SUCCESS(status)) {
		DDbgPrint(
			"  IoCallDriver IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION success\n");
	}
	else {
		DDbgPrint("  IoCallDriver IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION "
			"failed: 0x%x\n",
			status);
	}

	ExFreePool(targetName);

	DDbgPrint("<= DokanSendVolumeArrivalNotification\n");

	return status;
}

NTSTATUS
DokanSendVolumeDeletePoints(__in PUNICODE_STRING MountPoint,
	__in PUNICODE_STRING DeviceName) {
	NTSTATUS status;
	PMOUNTMGR_MOUNT_POINT point;
	PMOUNTMGR_MOUNT_POINTS deletedPoints;
	ULONG length;
	ULONG olength;

	DDbgPrint("=> DokanSendVolumeDeletePoints\n");

	length = sizeof(MOUNTMGR_MOUNT_POINT) + MountPoint->Length;
	if (DeviceName != NULL) {
		length += DeviceName->Length;
	}
	point = (PMOUNTMGR_MOUNT_POINT)ExAllocatePool(length);

	if (point == NULL) {
		DDbgPrint("  can't allocate MOUNTMGR_CREATE_POINT_INPUT\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	olength = sizeof(MOUNTMGR_MOUNT_POINTS) + 1024;
	deletedPoints = (PMOUNTMGR_MOUNT_POINTS)ExAllocatePool(olength);
	if (deletedPoints == NULL) {
		DDbgPrint("  can't allocate PMOUNTMGR_MOUNT_POINTS\n");
		ExFreePool(point);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(point, length);
	RtlZeroMemory(deletedPoints, olength);

	DDbgPrint("  MountPoint: %wZ\n", MountPoint);
	point->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
	point->SymbolicLinkNameLength = MountPoint->Length;
	RtlCopyMemory((PCHAR)point + point->SymbolicLinkNameOffset,
		MountPoint->Buffer, MountPoint->Length);
	if (DeviceName != NULL) {
		DDbgPrint("  DeviceName: %wZ\n", DeviceName);
		point->DeviceNameOffset =
			point->SymbolicLinkNameOffset + point->SymbolicLinkNameLength;
		point->DeviceNameLength = DeviceName->Length;
		RtlCopyMemory((PCHAR)point + point->DeviceNameOffset, DeviceName->Buffer,
			DeviceName->Length);
	}

	status = DokanSendIoContlToMountManager(IOCTL_MOUNTMGR_DELETE_POINTS, point,
		length, deletedPoints, olength);

	if (NT_SUCCESS(status)) {
		DDbgPrint("  IoCallDriver success, %d mount points deleted.\n",
			deletedPoints->NumberOfMountPoints);
	}
	else {
		DDbgPrint("  IoCallDriver failed: 0x%x\n", status);
	}

	ExFreePool(point);
	ExFreePool(deletedPoints);

	DDbgPrint("<= DokanSendVolumeDeletePoints\n");

	return status;
}

NTSTATUS
DokanSendVolumeCreatePoint(__in PUNICODE_STRING DeviceName,
	__in PUNICODE_STRING MountPoint) {
	NTSTATUS status;
	PMOUNTMGR_CREATE_POINT_INPUT point;
	ULONG length;

	DDbgPrint("=> DokanSendVolumeCreatePoint\n");

	length = sizeof(MOUNTMGR_CREATE_POINT_INPUT) + MountPoint->Length +
		DeviceName->Length;
	point = (PMOUNTMGR_CREATE_POINT_INPUT)ExAllocatePool(length);

	if (point == NULL) {
		DDbgPrint("  can't allocate MOUNTMGR_CREATE_POINT_INPUT\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(point, length);

	DDbgPrint("  DeviceName: %wZ\n", DeviceName);
	point->DeviceNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
	point->DeviceNameLength = DeviceName->Length;
	RtlCopyMemory((PCHAR)point + point->DeviceNameOffset, DeviceName->Buffer,
		DeviceName->Length);

	DDbgPrint("  MountPoint: %wZ\n", MountPoint);
	point->SymbolicLinkNameOffset =
		point->DeviceNameOffset + point->DeviceNameLength;
	point->SymbolicLinkNameLength = MountPoint->Length;
	RtlCopyMemory((PCHAR)point + point->SymbolicLinkNameOffset,
		MountPoint->Buffer, MountPoint->Length);

	status = DokanSendIoContlToMountManager(IOCTL_MOUNTMGR_CREATE_POINT, point,
		length, NULL, 0);

	if (NT_SUCCESS(status)) {
		DDbgPrint("  IoCallDriver success\n");
	}
	else {
		DDbgPrint("  IoCallDriver failed: 0x%x\n", status);
	}

	ExFreePool(point);

	DDbgPrint("<= DokanSendVolumeCreatePoint\n");

	return status;
}
