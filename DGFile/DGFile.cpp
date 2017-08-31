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
#include "DGFile.h"

// #ifdef ALLOC_PRAGMA
// #pragma alloc_text(INIT, DriverEntry)
// #pragma alloc_text(PAGE, DokanUnload)
// #endif

ULONG g_Debug = DOKAN_DEBUG_DEFAULT;
LOOKASIDE_LIST_EX g_DokanCCBLookasideList;
LOOKASIDE_LIST_EX g_DokanFCBLookasideList;

#if _WIN32_WINNT < 0x0501
PFN_FSRTLTEARDOWNPERSTREAMCONTEXTS DokanFsRtlTeardownPerStreamContexts;
#endif

NPAGED_LOOKASIDE_LIST DokanIrpEntryLookasideList;
UNICODE_STRING FcbFileNameNull;

NTSTATUS
DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)

/*++

Routine Description:

This routine gets called by the system to initialize the driver.

Arguments:

DriverObject	- the system supplied driver object.
RegistryPath	- the system supplied registry path for this driver.

Return Value:

NTSTATUS

--*/

{
	NTSTATUS status;
	PFAST_IO_DISPATCH fastIoDispatch;
	FS_FILTER_CALLBACKS filterCallbacks;
	PDOKAN_GLOBAL dokanGlobal = NULL;

	UNREFERENCED_PARAMETER(RegistryPath);

	DDbgPrint("==> DriverEntry ver.%x, %s %s\n", DOKAN_DRIVER_VERSION, __DATE__,
		__TIME__);

	status = DokanCreateGlobalDiskDevice(DriverObject, &dokanGlobal);

	if (NT_ERROR(status)) {
		return status;
	}
	//
	// Set up dispatch entry points for the driver.
	//
	DriverObject->DriverUnload = DokanUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DokanBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = DokanBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = DokanBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
		DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] =
		DokanBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_READ] = DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = DokanBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DokanBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = DokanBuildRequest;

	DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = DokanBuildRequest;
	DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] = DokanBuildRequest;

	fastIoDispatch = (PFAST_IO_DISPATCH)ExAllocatePool(sizeof(FAST_IO_DISPATCH));
	if (!fastIoDispatch) {
		IoDeleteDevice(dokanGlobal->FsDiskDeviceObject);
		IoDeleteDevice(dokanGlobal->FsCdDeviceObject);
		IoDeleteDevice(dokanGlobal->DeviceObject);
		DDbgPrint("  ExAllocatePool failed");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(fastIoDispatch, sizeof(FAST_IO_DISPATCH));

	fastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
	fastIoDispatch->FastIoCheckIfPossible = DokanFastIoCheckIfPossible;
	// fastIoDispatch->FastIoRead = DokanFastIoRead;
	fastIoDispatch->FastIoRead = FsRtlCopyRead;
	fastIoDispatch->FastIoWrite = FsRtlCopyWrite;
	fastIoDispatch->AcquireFileForNtCreateSection = DokanAcquireForCreateSection;
	fastIoDispatch->ReleaseFileForNtCreateSection = DokanReleaseForCreateSection;
	fastIoDispatch->MdlRead = FsRtlMdlReadDev;
	fastIoDispatch->MdlReadComplete = FsRtlMdlReadCompleteDev;
	fastIoDispatch->PrepareMdlWrite = FsRtlPrepareMdlWriteDev;
	fastIoDispatch->MdlWriteComplete = FsRtlMdlWriteCompleteDev;

	DriverObject->FastIoDispatch = fastIoDispatch;

	ExInitializeNPagedLookasideList(&DokanIrpEntryLookasideList, NULL, NULL, 0,
		sizeof(IRP_ENTRY), TAG, 0);

#if _WIN32_WINNT < 0x0501
	RtlInitUnicodeString(&functionName, L"FsRtlTeardownPerStreamContexts");
	DokanFsRtlTeardownPerStreamContexts =
		MmGetSystemRoutineAddress(&functionName);
#endif

	RtlZeroMemory(&filterCallbacks, sizeof(FS_FILTER_CALLBACKS));

	// only be used by filter driver?
	filterCallbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
	filterCallbacks.PreAcquireForSectionSynchronization =
		DokanFilterCallbackAcquireForCreateSection;

	status =
		FsRtlRegisterFileSystemFilterCallbacks(DriverObject, &filterCallbacks);

	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(dokanGlobal->FsDiskDeviceObject);
		IoDeleteDevice(dokanGlobal->FsCdDeviceObject);
		IoDeleteDevice(dokanGlobal->DeviceObject);
		DDbgPrint("  FsRtlRegisterFileSystemFilterCallbacks returned 0x%x\n",
			status);
		return status;
	}

	if (!DokanLookasideCreate(&g_DokanCCBLookasideList, sizeof(DokanCCB))) {
		IoDeleteDevice(dokanGlobal->FsDiskDeviceObject);
		IoDeleteDevice(dokanGlobal->FsCdDeviceObject);
		IoDeleteDevice(dokanGlobal->DeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	if (!DokanLookasideCreate(&g_DokanFCBLookasideList, sizeof(DokanFCB))) {
		IoDeleteDevice(dokanGlobal->FsDiskDeviceObject);
		IoDeleteDevice(dokanGlobal->FsCdDeviceObject);
		IoDeleteDevice(dokanGlobal->DeviceObject);
		ExDeleteLookasideListEx(&g_DokanCCBLookasideList);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	DDbgPrint("<== DriverEntry\n");

	return (status);
}

VOID DokanUnload(__in PDRIVER_OBJECT DriverObject)
/*++

Routine Description:

This routine gets called to remove the driver from the system.

Arguments:

DriverObject	- the system supplied driver object.

Return Value:

NTSTATUS

--*/

{

	PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
	WCHAR symbolicLinkBuf[] = DOKAN_GLOBAL_SYMBOLIC_LINK_NAME;
	UNICODE_STRING symbolicLinkName;
	PDOKAN_GLOBAL dokanGlobal;

	// PAGED_CODE();
	DDbgPrint("==> DokanUnload\n");

	dokanGlobal = (PDOKAN_GLOBAL)deviceObject->DeviceExtension;
	if (GetIdentifierType(dokanGlobal) == DGL) {
		DDbgPrint("  Delete Global DeviceObject\n");

		RtlInitUnicodeString(&symbolicLinkName, symbolicLinkBuf);
		IoDeleteSymbolicLink(&symbolicLinkName);

		IoUnregisterFileSystem(dokanGlobal->FsDiskDeviceObject);
		IoUnregisterFileSystem(dokanGlobal->FsCdDeviceObject);

		IoDeleteDevice(dokanGlobal->FsDiskDeviceObject);
		IoDeleteDevice(dokanGlobal->FsCdDeviceObject);
		IoDeleteDevice(deviceObject);
	}

	ExDeleteNPagedLookasideList(&DokanIrpEntryLookasideList);

	ExDeleteLookasideListEx(&g_DokanCCBLookasideList);
	ExDeleteLookasideListEx(&g_DokanFCBLookasideList);

	DDbgPrint("<== DokanUnload\n");
	return;
}
