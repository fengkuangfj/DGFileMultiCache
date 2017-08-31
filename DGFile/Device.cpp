
/*
Dokan : user-mode file system library for Windows

Copyright (C) 2015 - 2016 Adrien J. <liryna.stark@gmail.com> and Maxime C. <maxime@islog.com>
Copyright (C) 2007 - 2011 Hiroki Asakawa <info@dokan-dev.net>

http://dokan-dev.github.io

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Stdafx.h"
#include "Device.h"

NTSTATUS
DokanRegisterMountedDeviceInterface(__in PDEVICE_OBJECT DeviceObject,
	__in PDokanDCB Dcb) {
	NTSTATUS status;
	UNICODE_STRING interfaceName;
	DDbgPrint("=> DokanRegisterMountedDeviceInterface\n");

	status = IoRegisterDeviceInterface(
		DeviceObject, &MOUNTDEV_MOUNTED_DEVICE_GUID, NULL, &interfaceName);

	if (NT_SUCCESS(status)) {
		DDbgPrint("  InterfaceName:%wZ\n", &interfaceName);

		Dcb->MountedDeviceInterfaceName = interfaceName;
		status = IoSetDeviceInterfaceState(&interfaceName, TRUE);

		if (!NT_SUCCESS(status)) {
			DDbgPrint("  IoSetDeviceInterfaceState failed: 0x%x\n", status);
			RtlFreeUnicodeString(&interfaceName);
		}
	}
	else {
		DDbgPrint("  IoRegisterDeviceInterface failed: 0x%x\n", status);
	}

	if (!NT_SUCCESS(status)) {
		RtlInitUnicodeString(&(Dcb->MountedDeviceInterfaceName), NULL);
	}
	DDbgPrint("<= DokanRegisterMountedDeviceInterface\n");
	return status;
}

NTSTATUS
DokanRegisterDeviceInterface(__in PDRIVER_OBJECT DriverObject,
	__in PDEVICE_OBJECT DeviceObject,
	__in PDokanDCB Dcb) {
	PDEVICE_OBJECT pnpDeviceObject = NULL;
	NTSTATUS status;
	DDbgPrint("=> DokanRegisterDeviceInterface\n");

	status = IoReportDetectedDevice(DriverObject, InterfaceTypeUndefined, 0, 0,
		NULL, NULL, FALSE, &pnpDeviceObject);
	pnpDeviceObject->DeviceExtension = Dcb;
	if (NT_SUCCESS(status)) {
		DDbgPrint("  IoReportDetectedDevice success\n");
	}
	else {
		DDbgPrint("  IoReportDetectedDevice failed: 0x%x\n", status);
		return status;
	}

	if (IoAttachDeviceToDeviceStack(pnpDeviceObject, DeviceObject) != NULL) {
		DDbgPrint("  IoAttachDeviceToDeviceStack success\n");
	}
	else {
		DDbgPrint("  IoAttachDeviceToDeviceStack failed\n");
	}

	status = IoRegisterDeviceInterface(pnpDeviceObject, &GUID_DEVINTERFACE_DISK,
		NULL, &Dcb->DiskDeviceInterfaceName);

	if (NT_SUCCESS(status)) {
		DDbgPrint("  IoRegisterDeviceInterface success: %wZ\n",
			&Dcb->DiskDeviceInterfaceName);
	}
	else {
		RtlInitUnicodeString(&Dcb->DiskDeviceInterfaceName, NULL);
		DDbgPrint("  IoRegisterDeviceInterface failed: 0x%x\n", status);
		return status;
	}

	status = IoSetDeviceInterfaceState(&Dcb->DiskDeviceInterfaceName, TRUE);

	if (NT_SUCCESS(status)) {
		DDbgPrint("  IoSetDeviceInterfaceState success\n");
	}
	else {
		DDbgPrint("  IoSetDeviceInterfaceState failed: 0x%x\n", status);
		return status;
	}

	status =
		IoRegisterDeviceInterface(pnpDeviceObject, &MOUNTDEV_MOUNTED_DEVICE_GUID,
			NULL, &Dcb->MountedDeviceInterfaceName);

	if (NT_SUCCESS(status)) {
		DDbgPrint("  IoRegisterDeviceInterface success: %wZ\n",
			&Dcb->MountedDeviceInterfaceName);
	}
	else {
		DDbgPrint("  IoRegisterDeviceInterface failed: 0x%x\n", status);
		return status;
	}

	status = IoSetDeviceInterfaceState(&Dcb->MountedDeviceInterfaceName, TRUE);

	if (NT_SUCCESS(status)) {
		DDbgPrint("  IoSetDeviceInterfaceState success\n");
	}
	else {
		RtlInitUnicodeString(&Dcb->MountedDeviceInterfaceName, NULL);
		DDbgPrint("  IoSetDeviceInterfaceState failed: 0x%x\n", status);
		return status;
	}

	DDbgPrint("<= DokanRegisterDeviceInterface\n");
	return status;
}

NTSTATUS
DokanCreateGlobalDiskDevice(__in PDRIVER_OBJECT DriverObject,
	__out PDOKAN_GLOBAL *DokanGlobal) {

	NTSTATUS status;
	UNICODE_STRING deviceName;
	UNICODE_STRING symbolicLinkName;
	UNICODE_STRING fsDiskDeviceName;
	UNICODE_STRING fsCdDeviceName;
	PDEVICE_OBJECT deviceObject;
	PDEVICE_OBJECT fsDiskDeviceObject;
	PDEVICE_OBJECT fsCdDeviceObject;
	PDOKAN_GLOBAL dokanGlobal;

	RtlInitUnicodeString(&deviceName, DOKAN_GLOBAL_DEVICE_NAME);
	RtlInitUnicodeString(&symbolicLinkName, DOKAN_GLOBAL_SYMBOLIC_LINK_NAME);
	RtlInitUnicodeString(&fsDiskDeviceName, DOKAN_GLOBAL_FS_DISK_DEVICE_NAME);
	RtlInitUnicodeString(&fsCdDeviceName, DOKAN_GLOBAL_FS_CD_DEVICE_NAME);

	status = IoCreateDeviceSecure(DriverObject,         // DriverObject
		sizeof(DOKAN_GLOBAL), // DeviceExtensionSize
		&deviceName,          // DeviceName
		FILE_DEVICE_UNKNOWN,  // DeviceType
		0,                    // DeviceCharacteristics
		FALSE,                // Not Exclusive
		&sddl,                // Default SDDL String
		NULL,                 // Device Class GUID
		&deviceObject);       // DeviceObject

	if (!NT_SUCCESS(status)) {
		DDbgPrint("  IoCreateDevice returned 0x%x\n", status);
		return status;
	}
	DDbgPrint("DokanGlobalDevice: %wZ created\n", &deviceName);

	// Create supported file system device types and register them

	status = IoCreateDeviceSecure(DriverObject,      // DriverObject
		0,                 // DeviceExtensionSize
		&fsDiskDeviceName, // DeviceName
		FILE_DEVICE_DISK_FILE_SYSTEM, // DeviceType
		0,                    // DeviceCharacteristics
		FALSE,                // Not Exclusive
		&sddl,                // Default SDDL String
		NULL,                 // Device Class GUID
		&fsDiskDeviceObject); // DeviceObject

	if (!NT_SUCCESS(status)) {
		DDbgPrint("  IoCreateDevice Disk FileSystem failed: 0x%x\n", status);
		IoDeleteDevice(deviceObject);
		return status;
	}
	DDbgPrint("DokanDiskFileSystemDevice: %wZ created\n", &fsDiskDeviceName);

	status = IoCreateDeviceSecure(DriverObject,    // DriverObject
		0,               // DeviceExtensionSize
		&fsCdDeviceName, // DeviceName
		FILE_DEVICE_CD_ROM_FILE_SYSTEM, // DeviceType
		0,                  // DeviceCharacteristics
		FALSE,              // Not Exclusive
		&sddl,              // Default SDDL String
		NULL,               // Device Class GUID
		&fsCdDeviceObject); // DeviceObject

	if (!NT_SUCCESS(status)) {
		DDbgPrint("  IoCreateDevice Cd FileSystem failed: 0x%x\n", status);
		IoDeleteDevice(fsDiskDeviceObject);
		IoDeleteDevice(deviceObject);
		return status;
	}
	DDbgPrint("DokanCdFileSystemDevice: %wZ created\n", &fsCdDeviceName);

	ObReferenceObject(deviceObject);

	status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
	if (!NT_SUCCESS(status)) {
		DDbgPrint("  IoCreateSymbolicLink returned 0x%x\n", status);
		IoDeleteDevice(deviceObject);
		return status;
	}
	DDbgPrint("SymbolicLink: %wZ -> %wZ created\n", &deviceName,
		&symbolicLinkName);
	dokanGlobal = (PDOKAN_GLOBAL)deviceObject->DeviceExtension;
	dokanGlobal->DeviceObject = deviceObject;
	dokanGlobal->FsDiskDeviceObject = fsDiskDeviceObject;
	dokanGlobal->FsCdDeviceObject = fsCdDeviceObject;

	RtlZeroMemory(dokanGlobal, sizeof(DOKAN_GLOBAL));
	DokanInitIrpList(&dokanGlobal->PendingService);
	DokanInitIrpList(&dokanGlobal->NotifyService);
	InitializeListHead(&dokanGlobal->MountPointList);

	dokanGlobal->Identifier.Type = DGL;
	dokanGlobal->Identifier.Size = sizeof(DOKAN_GLOBAL);

	//
	// Establish user-buffer access method.
	//
	fsDiskDeviceObject->Flags |= DO_DIRECT_IO;
	fsDiskDeviceObject->Flags |= DO_LOW_PRIORITY_FILESYSTEM;
	fsCdDeviceObject->Flags |= DO_DIRECT_IO;
	fsCdDeviceObject->Flags |= DO_LOW_PRIORITY_FILESYSTEM;

	fsDiskDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	fsCdDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	// Register file systems
	IoRegisterFileSystem(fsDiskDeviceObject);
	IoRegisterFileSystem(fsCdDeviceObject);

	ObReferenceObject(fsDiskDeviceObject);
	ObReferenceObject(fsCdDeviceObject);

	*DokanGlobal = dokanGlobal;
	return STATUS_SUCCESS;
}

NTSTATUS
DokanCreateDiskDevice(__in PDRIVER_OBJECT DriverObject, __in ULONG MountId,
	__in PWCHAR MountPoint, __in PWCHAR UNCName,
	__in PWCHAR BaseGuid, __in PDOKAN_GLOBAL DokanGlobal,
	__in DEVICE_TYPE DeviceType,
	__in ULONG DeviceCharacteristics,
	__in BOOLEAN MountGlobally, __in BOOLEAN UseMountManager,
	__out PDokanDCB *Dcb) {
	WCHAR diskDeviceNameBuf[MAXIMUM_FILENAME_LENGTH];
	WCHAR symbolicLinkNameBuf[MAXIMUM_FILENAME_LENGTH];
	WCHAR mountPointBuf[MAXIMUM_FILENAME_LENGTH];
	PDEVICE_OBJECT diskDeviceObject;
	PDokanDCB dcb;
	UNICODE_STRING diskDeviceName;
	NTSTATUS status;
	BOOLEAN isNetworkFileSystem = (DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM);
	DOKAN_CONTROL dokanControl;

	// make DeviceName and SymboliLink
	if (isNetworkFileSystem) {
#ifdef DOKAN_NET_PROVIDER
		RtlStringCchCopyW(diskDeviceNameBuf, MAXIMUM_FILENAME_LENGTH,
			DOKAN_NET_DEVICE_NAME);
		RtlStringCchCopyW(symbolicLinkNameBuf, MAXIMUM_FILENAME_LENGTH,
			DOKAN_NET_SYMBOLIC_LINK_NAME);
#else
		RtlStringCchCopyW(diskDeviceNameBuf, MAXIMUM_FILENAME_LENGTH,
			DOKAN_NET_DEVICE_NAME);
		RtlStringCchCatW(diskDeviceNameBuf, MAXIMUM_FILENAME_LENGTH, BaseGuid);
		RtlStringCchCopyW(symbolicLinkNameBuf, MAXIMUM_FILENAME_LENGTH,
			DOKAN_NET_SYMBOLIC_LINK_NAME);
		RtlStringCchCatW(symbolicLinkNameBuf, MAXIMUM_FILENAME_LENGTH, BaseGuid);
#endif

	}
	else {
		RtlStringCchCopyW(diskDeviceNameBuf, MAXIMUM_FILENAME_LENGTH,
			DOKAN_DISK_DEVICE_NAME);
		RtlStringCchCatW(diskDeviceNameBuf, MAXIMUM_FILENAME_LENGTH, BaseGuid);
		RtlStringCchCopyW(symbolicLinkNameBuf, MAXIMUM_FILENAME_LENGTH,
			DOKAN_SYMBOLIC_LINK_NAME);
		RtlStringCchCatW(symbolicLinkNameBuf, MAXIMUM_FILENAME_LENGTH, BaseGuid);
	}

	RtlInitUnicodeString(&diskDeviceName, diskDeviceNameBuf);

	//
	// Create DeviceObject for the Disk Device
	//
	if (!isNetworkFileSystem) {
		status =
			IoCreateDeviceSecure(DriverObject,          // DriverObject
				sizeof(DokanDCB),      // DeviceExtensionSize
				&diskDeviceName,       // DeviceName
				FILE_DEVICE_DISK,      // DeviceType
				DeviceCharacteristics, // DeviceCharacteristics
				FALSE,                 // Not Exclusive
				&sddl,                 // Default SDDL String
				NULL,                  // Device Class GUID
				&diskDeviceObject);    // DeviceObject
	}
	else {
		status = IoCreateDevice(DriverObject,          // DriverObject
			sizeof(DokanDCB),      // DeviceExtensionSize
			NULL,                  // DeviceName
			FILE_DEVICE_DISK,      // DeviceType
			DeviceCharacteristics, // DeviceCharacteristics
			FALSE,                 // Not Exclusive
			&diskDeviceObject);    // DeviceObject
	}

	if (!NT_SUCCESS(status)) {
		DDbgPrint("  %s failed: 0x%x\n",
			isNetworkFileSystem ? "IoCreateDevice(FILE_DEVICE_UNKNOWN)"
			: "IoCreateDeviceSecure(FILE_DEVICE_DISK)",
			status);
		return status;
	}

	//
	// Initialize the device extension.
	//
	dcb = (PDokanDCB)diskDeviceObject->DeviceExtension;
	*Dcb = dcb;
	dcb->DeviceObject = diskDeviceObject;
	dcb->Global = DokanGlobal;

	dcb->Identifier.Type = DCB;
	dcb->Identifier.Size = sizeof(DokanDCB);

	dcb->MountId = MountId;
	dcb->VolumeDeviceType = DeviceType;
	dcb->DeviceType = FILE_DEVICE_DISK;
	dcb->DeviceCharacteristics = DeviceCharacteristics;
	KeInitializeEvent(&dcb->KillEvent, NotificationEvent, FALSE);

	//
	// Establish user-buffer access method.
	//
	diskDeviceObject->Flags |= DO_DIRECT_IO;

	// initialize Event and Event queue
	DokanInitIrpList(&dcb->PendingIrp);
	DokanInitIrpList(&dcb->PendingEvent);
	DokanInitIrpList(&dcb->NotifyEvent);

	KeInitializeEvent(&dcb->ReleaseEvent, NotificationEvent, FALSE);

	// "0" means not mounted
	dcb->Mounted = 0;

	ExInitializeResourceLite(&dcb->Resource);

	dcb->CacheManagerNoOpCallbacks.AcquireForLazyWrite = &DokanNoOpAcquire;
	dcb->CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = &DokanNoOpRelease;
	dcb->CacheManagerNoOpCallbacks.AcquireForReadAhead = &DokanNoOpAcquire;
	dcb->CacheManagerNoOpCallbacks.ReleaseFromReadAhead = &DokanNoOpRelease;

	dcb->MountGlobally = MountGlobally;
	dcb->UseMountManager = UseMountManager;
	if (wcscmp(MountPoint, L"") != 0) {
		RtlStringCchCopyW(mountPointBuf, MAXIMUM_FILENAME_LENGTH,
			L"\\DosDevices\\");
		if (wcslen(MountPoint) < 4) {
			mountPointBuf[12] = towupper(MountPoint[0]);
			mountPointBuf[13] = L':';
			mountPointBuf[14] = L'\0';
			if (isNetworkFileSystem) {
				dcb->UseMountManager = FALSE;
			}
		}
		else {
			dcb->UseMountManager = FALSE;
			RtlStringCchCatW(mountPointBuf, MAXIMUM_FILENAME_LENGTH, MountPoint);
		}
	}
	else {
		RtlStringCchCopyW(mountPointBuf, MAXIMUM_FILENAME_LENGTH, L"");
	}

	dcb->DiskDeviceName = DokanAllocateUnicodeString(diskDeviceNameBuf);
	dcb->SymbolicLinkName = DokanAllocateUnicodeString(symbolicLinkNameBuf);
	dcb->MountPoint = DokanAllocateUnicodeString(mountPointBuf);
	if (UNCName != NULL) {
		dcb->UNCName = DokanAllocateUnicodeString(UNCName);
	}

	if (dcb->DiskDeviceName == NULL || dcb->SymbolicLinkName == NULL ||
		dcb->MountPoint == NULL || (dcb->UNCName == NULL && UNCName != NULL)) {
		DDbgPrint("  Failed to allocate memory for device naming");
		FreeDcbNames(dcb);
		ExDeleteResourceLite(&dcb->Resource);
		IoDeleteDevice(diskDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	DDbgPrint("DiskDeviceName: %wZ - SymbolicLinkName: %wZ - MountPoint: %wZ\n",
		dcb->DiskDeviceName, dcb->SymbolicLinkName, dcb->MountPoint);

	DDbgPrint("  IoCreateDevice DeviceType: %d\n", DeviceType);

	//
	// Create a symbolic link for userapp to interact with the driver.
	//
	status = IoCreateSymbolicLink(dcb->SymbolicLinkName, dcb->DiskDeviceName);

	if (!NT_SUCCESS(status)) {
		ExDeleteResourceLite(&dcb->Resource);
		IoDeleteDevice(diskDeviceObject);
		FreeDcbNames(dcb);
		DDbgPrint("  IoCreateSymbolicLink returned 0x%x\n", status);
		return status;
	}
	DDbgPrint("SymbolicLink: %wZ -> %wZ created\n", dcb->SymbolicLinkName,
		dcb->DiskDeviceName);

	// Mark devices as initialized
	diskDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	ObReferenceObject(diskDeviceObject);

	// DokanRegisterDeviceInterface(DriverObject, dcb->DeviceObject, dcb);
	// DokanRegisterMountedDeviceInterface(dcb->DeviceObject, dcb);

	// Save to the global mounted list
	RtlZeroMemory(&dokanControl, sizeof(dokanControl));
	RtlStringCchCopyW(dokanControl.DeviceName,
		sizeof(dokanControl.DeviceName) / sizeof(WCHAR),
		diskDeviceNameBuf);
	RtlStringCchCopyW(dokanControl.MountPoint,
		sizeof(dokanControl.MountPoint) / sizeof(WCHAR),
		mountPointBuf);
	if (UNCName != NULL) {
		RtlStringCchCopyW(dokanControl.UNCName,
			sizeof(dokanControl.UNCName) / sizeof(WCHAR), UNCName);
	}
	dokanControl.Type = DeviceType;

	InsertMountEntry(DokanGlobal, &dokanControl);

	IoVerifyVolume(dcb->DeviceObject, FALSE);

	return STATUS_SUCCESS;
}

VOID DokanDeleteDeviceObject(__in PDokanDCB Dcb) {
	PDokanVCB vcb;
	DOKAN_CONTROL dokanControl;
	PMOUNT_ENTRY mountEntry = NULL;

	ASSERT(GetIdentifierType(Dcb) == DCB);
	vcb = (PDokanVCB)Dcb->Vcb;

	if (Dcb->SymbolicLinkName == NULL) {
		DDbgPrint("  Symbolic Name already deleted, so go out here\n");
		return;
	}

	RtlZeroMemory(&dokanControl, sizeof(dokanControl));
	RtlCopyMemory(dokanControl.DeviceName, Dcb->DiskDeviceName->Buffer,
		Dcb->DiskDeviceName->Length);
	mountEntry = FindMountEntry(Dcb->Global, &dokanControl);
	if (mountEntry != NULL) {
		if (mountEntry->MountControl.Type == FILE_DEVICE_NETWORK_FILE_SYSTEM) {
			// Run FsRtlDeregisterUncProvider in System thread.
			HANDLE handle;
			PKTHREAD thread;
			OBJECT_ATTRIBUTES objectAttribs;
			NTSTATUS status;

			InitializeObjectAttributes(&objectAttribs, NULL, OBJ_KERNEL_HANDLE, NULL,
				NULL);
			status = PsCreateSystemThread(
				&handle, THREAD_ALL_ACCESS, &objectAttribs, NULL, NULL,
				(PKSTART_ROUTINE)DokanDeregisterUncProvider, Dcb);
			if (!NT_SUCCESS(status)) {
				DDbgPrint("PsCreateSystemThread failed: 0x%X\n", status);
			}
			else {
				ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, NULL, KernelMode,
					(PVOID *)&thread, NULL);
				ZwClose(handle);
				KeWaitForSingleObject(thread, Executive, KernelMode, FALSE, NULL);
				ObDereferenceObject(thread);
			}
		}
		RemoveMountEntry(Dcb->Global, mountEntry);
	}
	else {
		DDbgPrint("  Cannot found associated mount entry.\n");
	}

	DDbgPrint("  Delete Symbolic Name: %wZ\n", Dcb->SymbolicLinkName);
	IoDeleteSymbolicLink(Dcb->SymbolicLinkName);

	if (Dcb->MountedDeviceInterfaceName.Buffer != NULL) {
		IoSetDeviceInterfaceState(&Dcb->MountedDeviceInterfaceName, FALSE);

		RtlFreeUnicodeString(&Dcb->MountedDeviceInterfaceName);
		RtlInitUnicodeString(&Dcb->MountedDeviceInterfaceName, NULL);
	}
	if (Dcb->DiskDeviceInterfaceName.Buffer != NULL) {
		IoSetDeviceInterfaceState(&Dcb->DiskDeviceInterfaceName, FALSE);

		RtlFreeUnicodeString(&Dcb->DiskDeviceInterfaceName);
		RtlInitUnicodeString(&Dcb->DiskDeviceInterfaceName, NULL);
	}

	FreeDcbNames(Dcb);

	if (Dcb->DeviceObject->Vpb) {
		Dcb->DeviceObject->Vpb->DeviceObject = NULL;
		Dcb->DeviceObject->Vpb->RealDevice = NULL;
		Dcb->DeviceObject->Vpb->Flags = 0;
	}

	if (vcb != NULL) {
		DDbgPrint("  FCB allocated: %d\n", vcb->FcbAllocated);
		DDbgPrint("  FCB     freed: %d\n", vcb->FcbFreed);
		DDbgPrint("  CCB allocated: %d\n", vcb->CcbAllocated);
		DDbgPrint("  CCB     freed: %d\n", vcb->CcbFreed);

		// delete volDeviceObject
		DDbgPrint("  Delete Volume DeviceObject\n");
		IoDeleteDevice(vcb->DeviceObject);
	}

	// delete diskDeviceObject
	DDbgPrint("  Delete Disk DeviceObject\n");
	IoDeleteDevice(Dcb->DeviceObject);
}

NTSTATUS
QueryDeviceRelations(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	DEVICE_RELATION_TYPE type = irpSp->Parameters.QueryDeviceRelations.Type;
	PDEVICE_RELATIONS DeviceRelations;
	PDokanVCB vcb;

	vcb = (PDokanVCB)DeviceObject->DeviceExtension;

	switch (type) {
	case RemovalRelations:
		DDbgPrint("  QueryDeviceRelations - RemovalRelations\n");
		break;
	case TargetDeviceRelation:

		DDbgPrint("  QueryDeviceRelations - TargetDeviceRelation\n");

		DeviceRelations =
			(PDEVICE_RELATIONS)ExAllocatePool(sizeof(DEVICE_RELATIONS));
		if (!DeviceRelations) {
			DDbgPrint("  can't allocate DeviceRelations\n");
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		/* The PnP manager will remove this when it is done with device */
		ObReferenceObject(DeviceObject);

		DeviceRelations->Count = 1;
		DeviceRelations->Objects[0] = DeviceObject;
		Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

		return STATUS_SUCCESS;

	case EjectionRelations:
		DDbgPrint("  QueryDeviceRelations - EjectionRelations\n");
		break;
	case BusRelations:
		DDbgPrint("  QueryDeviceRelations - BusRelations\n");
		break;
	default:
		break;
	}

	return status;
}
