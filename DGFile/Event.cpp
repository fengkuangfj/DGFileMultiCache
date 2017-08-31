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
#include "Event.h"

// start event dispatching
NTSTATUS
DokanEventStart(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	ULONG outBufferLen;
	ULONG inBufferLen;
	PIO_STACK_LOCATION irpSp;
	EVENT_START eventStart;
	PEVENT_DRIVER_INFO driverInfo;
	PDOKAN_GLOBAL dokanGlobal;
	PDokanDCB dcb;
	NTSTATUS status;
	DEVICE_TYPE deviceType;
	ULONG deviceCharacteristics = 0;
	WCHAR baseGuidString[64];
	GUID baseGuid = DOKAN_BASE_GUID;
	UNICODE_STRING unicodeGuid;
	ULONG deviceNamePos;
	BOOLEAN useMountManager = FALSE;
	BOOLEAN mountGlobally = TRUE;
	BOOLEAN fileLockUserMode = FALSE;

	DDbgPrint("==> DokanEventStart\n");

	dokanGlobal = (PDOKAN_GLOBAL)DeviceObject->DeviceExtension;
	if (GetIdentifierType(dokanGlobal) != DGL) {
		return STATUS_INVALID_PARAMETER;
	}

	irpSp = IoGetCurrentIrpStackLocation(Irp);

	outBufferLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
	inBufferLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;

	if (outBufferLen != sizeof(EVENT_DRIVER_INFO) ||
		inBufferLen != sizeof(EVENT_START)) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlCopyMemory(&eventStart, Irp->AssociatedIrp.SystemBuffer,
		sizeof(EVENT_START));
	driverInfo = (PEVENT_DRIVER_INFO)Irp->AssociatedIrp.SystemBuffer;

	if (eventStart.UserVersion != DOKAN_DRIVER_VERSION) {
		driverInfo->DriverVersion = DOKAN_DRIVER_VERSION;
		driverInfo->Status = DOKAN_START_FAILED;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(EVENT_DRIVER_INFO);
		return STATUS_SUCCESS;
	}

	switch (eventStart.DeviceType) {
	case DOKAN_DISK_FILE_SYSTEM:
		deviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
		break;
	case DOKAN_NETWORK_FILE_SYSTEM:
		deviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
		deviceCharacteristics |= FILE_REMOTE_DEVICE;
		break;
	default:
		DDbgPrint("  Unknown device type: %d\n", eventStart.DeviceType);
		deviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
	}

	if (eventStart.Flags & DOKAN_EVENT_REMOVABLE) {
		DDbgPrint("  DeviceCharacteristics |= FILE_REMOVABLE_MEDIA\n");
		deviceCharacteristics |= FILE_REMOVABLE_MEDIA;
	}

	if (eventStart.Flags & DOKAN_EVENT_WRITE_PROTECT) {
		DDbgPrint("  DeviceCharacteristics |= FILE_READ_ONLY_DEVICE\n");
		deviceCharacteristics |= FILE_READ_ONLY_DEVICE;
	}

	if (eventStart.Flags & DOKAN_EVENT_MOUNT_MANAGER) {
		DDbgPrint("  Using Mount Manager\n");
		useMountManager = TRUE;
	}

	if (eventStart.Flags & DOKAN_EVENT_CURRENT_SESSION) {
		DDbgPrint("  Mounting on current session only\n");
		mountGlobally = FALSE;
	}

	if (eventStart.Flags & DOKAN_EVENT_FILELOCK_USER_MODE) {
		DDbgPrint("  FileLock in User Mode\n");
		fileLockUserMode = TRUE;
	}

	baseGuid.Data2 = (USHORT)(dokanGlobal->MountId & 0xFFFF) ^ baseGuid.Data2;
	baseGuid.Data3 = (USHORT)(dokanGlobal->MountId >> 16) ^ baseGuid.Data3;

	// const GUID baseGuidTemp = baseGuid;

	status = RtlStringFromGUID(baseGuid, &unicodeGuid);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	RtlZeroMemory(baseGuidString, sizeof(baseGuidString));
	RtlStringCchCopyW(baseGuidString, sizeof(baseGuidString) / sizeof(WCHAR),
		unicodeGuid.Buffer);
	RtlFreeUnicodeString(&unicodeGuid);

	InterlockedIncrement((LONG *)&dokanGlobal->MountId);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&dokanGlobal->Resource, TRUE);

	status = DokanCreateDiskDevice(
		DeviceObject->DriverObject, dokanGlobal->MountId, eventStart.MountPoint,
		eventStart.UNCName, baseGuidString, dokanGlobal, deviceType,
		deviceCharacteristics, mountGlobally, useMountManager, &dcb);

	if (!NT_SUCCESS(status)) {
		ExReleaseResourceLite(&dokanGlobal->Resource);
		KeLeaveCriticalRegion();
		return status;
	}

	dcb->FileLockInUserMode = fileLockUserMode;

	DDbgPrint("  MountId:%d\n", dcb->MountId);
	driverInfo->DeviceNumber = dokanGlobal->MountId;
	driverInfo->MountId = dokanGlobal->MountId;
	driverInfo->Status = DOKAN_MOUNTED;
	driverInfo->DriverVersion = DOKAN_DRIVER_VERSION;

	// SymbolicName is
	// \\DosDevices\\Global\\Volume{D6CC17C5-1734-4085-BCE7-964F1E9F5DE9}
	// Finds the last '\' and copy into DeviceName.
	// DeviceName is \Volume{D6CC17C5-1734-4085-BCE7-964F1E9F5DE9}
	deviceNamePos = dcb->SymbolicLinkName->Length / sizeof(WCHAR) - 1;
	for (; dcb->SymbolicLinkName->Buffer[deviceNamePos] != L'\\'; --deviceNamePos)
		;
	RtlStringCchCopyW(driverInfo->DeviceName,
		sizeof(driverInfo->DeviceName) / sizeof(WCHAR),
		&(dcb->SymbolicLinkName->Buffer[deviceNamePos]));

	// Set the irp timeout in milliseconds
	// If the IrpTimeout is 0, we assume that the value was not changed
	dcb->IrpTimeout = DOKAN_IRP_PENDING_TIMEOUT;
	if (eventStart.IrpTimeout > 0) {
		if (eventStart.IrpTimeout > DOKAN_IRP_PENDING_TIMEOUT_RESET_MAX) {
			eventStart.IrpTimeout = DOKAN_IRP_PENDING_TIMEOUT_RESET_MAX;
		}

		if (eventStart.IrpTimeout < DOKAN_IRP_PENDING_TIMEOUT) {
			eventStart.IrpTimeout = DOKAN_IRP_PENDING_TIMEOUT;
		}
		dcb->IrpTimeout = eventStart.IrpTimeout;
	}

	DDbgPrint("  DeviceName:%ws\n", driverInfo->DeviceName);

	dcb->UseAltStream = 0;
	if (eventStart.Flags & DOKAN_EVENT_ALTERNATIVE_STREAM_ON) {
		DDbgPrint("  ALT_STREAM_ON\n");
		dcb->UseAltStream = 1;
	}

	DokanStartEventNotificationThread(dcb);

	ExReleaseResourceLite(&dokanGlobal->Resource);
	KeLeaveCriticalRegion();

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = sizeof(EVENT_DRIVER_INFO);

	DDbgPrint("<== DokanEventStart\n");

	return Irp->IoStatus.Status;
}

// user assinged bigger buffer that is enough to return WriteEventContext
NTSTATUS
DokanEventWrite(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	KIRQL oldIrql;
	PLIST_ENTRY thisEntry, nextEntry, listHead;
	PIRP_ENTRY irpEntry;
	PDokanVCB vcb;
	PEVENT_INFORMATION eventInfo;
	PIRP writeIrp;

	eventInfo = (PEVENT_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
	ASSERT(eventInfo != NULL);

	DDbgPrint("==> DokanEventWrite [EventInfo #%X]\n", eventInfo->SerialNumber);

	vcb = (PDokanVCB)DeviceObject->DeviceExtension;

	if (GetIdentifierType(vcb) != VCB) {
		return STATUS_INVALID_PARAMETER;
	}

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeAcquireSpinLock(&vcb->Dcb->PendingIrp.ListLock, &oldIrql);

	// search corresponding write IRP through pending IRP list
	listHead = &vcb->Dcb->PendingIrp.ListHead;

	for (thisEntry = listHead->Flink; thisEntry != listHead;
		thisEntry = nextEntry) {

		PIO_STACK_LOCATION writeIrpSp, eventIrpSp;
		PEVENT_CONTEXT eventContext;
		ULONG info = 0;
		NTSTATUS status;

		nextEntry = thisEntry->Flink;
		irpEntry = CONTAINING_RECORD(thisEntry, IRP_ENTRY, ListEntry);

		// check whehter this is corresponding IRP

		// DDbgPrint("SerialNumber irpEntry %X eventInfo %X\n",
		// irpEntry->SerialNumber, eventInfo->SerialNumber);

		if (irpEntry->SerialNumber != eventInfo->SerialNumber) {
			continue;
		}

		// do NOT free irpEntry here
		writeIrp = irpEntry->Irp;
		if (writeIrp == NULL) {
			// this IRP has already been canceled
			ASSERT(irpEntry->CancelRoutineFreeMemory == FALSE);
			DokanFreeIrpEntry(irpEntry);
			continue;
		}

		if (IoSetCancelRoutine(writeIrp, DokanIrpCancelRoutine) == NULL) {
			// if (IoSetCancelRoutine(writeIrp, NULL) != NULL) {
			// Cancel routine will run as soon as we release the lock
			InitializeListHead(&irpEntry->ListEntry);
			irpEntry->CancelRoutineFreeMemory = TRUE;
			continue;
		}

		writeIrpSp = irpEntry->IrpSp;
		eventIrpSp = IoGetCurrentIrpStackLocation(Irp);

		ASSERT(writeIrpSp != NULL);
		ASSERT(eventIrpSp != NULL);

		eventContext =
			(PEVENT_CONTEXT)
			writeIrp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_EVENT];
		ASSERT(eventContext != NULL);

		// short of buffer length
		if (eventIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
			eventContext->Length) {
			DDbgPrint("  EventWrite: STATUS_INSUFFICIENT_RESOURCE\n");
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
		else {
			PVOID buffer;
			// DDbgPrint("  EventWrite CopyMemory\n");
			// DDbgPrint("  EventLength %d, BufLength %d\n", eventContext->Length,
			//			eventIrpSp->Parameters.DeviceIoControl.OutputBufferLength);
			if (Irp->MdlAddress)
				buffer =
				MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
			else
				buffer = Irp->AssociatedIrp.SystemBuffer;

			ASSERT(buffer != NULL);
			RtlCopyMemory(buffer, eventContext, eventContext->Length);

			info = eventContext->Length;
			status = STATUS_SUCCESS;
		}

		DokanFreeEventContext(eventContext);
		writeIrp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_EVENT] = 0;

		KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = info;

		// this IRP will be completed by caller function
		return Irp->IoStatus.Status;
	}

	KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);

	return STATUS_SUCCESS;
}

NTSTATUS DokanEventRelease(__in PDEVICE_OBJECT DeviceObject) {
	PDokanDCB dcb;
	PDokanVCB vcb;
	PDokanFCB fcb;
	PDokanCCB ccb;
	PLIST_ENTRY fcbEntry, fcbNext, fcbHead;
	PLIST_ENTRY ccbEntry, ccbNext, ccbHead;
	NTSTATUS status = STATUS_SUCCESS;

	DDbgPrint("==> DokanEventRelease\n");

	if (DeviceObject == NULL) {
		return STATUS_INVALID_PARAMETER;
	}

	vcb = (PDokanVCB)DeviceObject->DeviceExtension;
	if (GetIdentifierType(vcb) != VCB) {
		return STATUS_INVALID_PARAMETER;
	}
	dcb = vcb->Dcb;

	DokanDeleteMountPoint(dcb);

	// ExAcquireResourceExclusiveLite(&dcb->Resource, TRUE);
	dcb->Mounted = 0;
	// ExReleaseResourceLite(&dcb->Resource);

	// search CCB list to complete not completed Directory Notification

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&vcb->Resource, TRUE);

	fcbHead = &vcb->NextFCB;

	for (fcbEntry = fcbHead->Flink; fcbEntry != fcbHead; fcbEntry = fcbNext) {

		fcbNext = fcbEntry->Flink;
		fcb = CONTAINING_RECORD(fcbEntry, DokanFCB, NextFCB);

		ExAcquireResourceExclusiveLite(&fcb->Resource, TRUE);

		ccbHead = &fcb->NextCCB;

		for (ccbEntry = ccbHead->Flink; ccbEntry != ccbHead; ccbEntry = ccbNext) {
			ccbNext = ccbEntry->Flink;
			ccb = CONTAINING_RECORD(ccbEntry, DokanCCB, NextCCB);

			DDbgPrint("  NotifyCleanup ccb:%p, context:%X, filename:%wZ\n", ccb,
				(ULONG)ccb->UserContext, &fcb->FileName);
			FsRtlNotifyCleanup(vcb->NotifySync, &vcb->DirNotifyList, ccb);
		}
		ExReleaseResourceLite(&fcb->Resource);
	}

	ExReleaseResourceLite(&vcb->Resource);
	KeLeaveCriticalRegion();

	ReleasePendingIrp(&dcb->PendingIrp);
	ReleasePendingIrp(&dcb->PendingEvent);
	DokanStopCheckThread(dcb);
	DokanStopEventNotificationThread(dcb);

	DokanDeleteDeviceObject(dcb);

	DDbgPrint("<== DokanEventRelease\n");

	return status;
}

NTSTATUS DokanGlobalEventRelease(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp) {
	PDOKAN_GLOBAL dokanGlobal;
	PIO_STACK_LOCATION irpSp;
	PDOKAN_UNICODE_STRING_INTERMEDIATE szMountPoint;
	DOKAN_CONTROL dokanControl;
	PMOUNT_ENTRY mountEntry;

	dokanGlobal = (PDOKAN_GLOBAL)DeviceObject->DeviceExtension;
	if (GetIdentifierType(dokanGlobal) != DGL) {
		return STATUS_INVALID_PARAMETER;
	}

	irpSp = IoGetCurrentIrpStackLocation(Irp);

	if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
		sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE)) {
		DDbgPrint(
			"Input buffer is too small (< DOKAN_UNICODE_STRING_INTERMEDIATE)\n");
		return STATUS_BUFFER_TOO_SMALL;
	}
	szMountPoint =
		(PDOKAN_UNICODE_STRING_INTERMEDIATE)Irp->AssociatedIrp.SystemBuffer;
	if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
		sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE) + szMountPoint->MaximumLength) {
		DDbgPrint("Input buffer is too small\n");
		return STATUS_BUFFER_TOO_SMALL;
	}

	RtlZeroMemory(&dokanControl, sizeof(dokanControl));
	RtlStringCchCopyW(dokanControl.MountPoint, MAXIMUM_FILENAME_LENGTH,
		L"\\DosDevices\\");
	if ((szMountPoint->Length / sizeof(WCHAR)) < 4) {
		dokanControl.MountPoint[12] = towupper(szMountPoint->Buffer[0]);
		dokanControl.MountPoint[13] = L':';
		dokanControl.MountPoint[14] = L'\0';
	}
	else {
		RtlCopyMemory(&dokanControl.MountPoint[12], szMountPoint->Buffer,
			szMountPoint->Length);
	}
	mountEntry = FindMountEntry(dokanGlobal, &dokanControl);
	if (mountEntry == NULL) {
		DDbgPrint("Cannot found device associated to mount point %ws\n",
			dokanControl.MountPoint);
		return STATUS_BUFFER_TOO_SMALL;
	}

	return DokanEventRelease(mountEntry->MountControl.DeviceObject);
}

VOID SetCommonEventContext(__in PDokanDCB Dcb, __in PEVENT_CONTEXT EventContext,
	__in PIRP Irp, __in_opt PDokanCCB Ccb) {
	PIO_STACK_LOCATION irpSp;

	irpSp = IoGetCurrentIrpStackLocation(Irp);

	EventContext->MountId = Dcb->MountId;
	EventContext->MajorFunction = irpSp->MajorFunction;
	EventContext->MinorFunction = irpSp->MinorFunction;
	EventContext->Flags = irpSp->Flags;

	if (Ccb) {
		EventContext->FileFlags = Ccb->Flags;
	}

	EventContext->ProcessId = IoGetRequestorProcessId(Irp);
}

PEVENT_CONTEXT
AllocateEventContextRaw(__in ULONG EventContextLength) {
	ULONG driverContextLength;
	PDRIVER_EVENT_CONTEXT driverEventContext;
	PEVENT_CONTEXT eventContext;

	driverContextLength =
		EventContextLength - sizeof(EVENT_CONTEXT) + sizeof(DRIVER_EVENT_CONTEXT);
	driverEventContext = (PDRIVER_EVENT_CONTEXT)ExAllocatePool(driverContextLength);

	if (driverEventContext == NULL) {
		return NULL;
	}

	RtlZeroMemory(driverEventContext, driverContextLength);
	InitializeListHead(&driverEventContext->ListEntry);

	eventContext = &driverEventContext->EventContext;
	eventContext->Length = EventContextLength;

	return eventContext;
}

PEVENT_CONTEXT
AllocateEventContext(__in PDokanDCB Dcb, __in PIRP Irp,
	__in ULONG EventContextLength, __in_opt PDokanCCB Ccb) {
	PEVENT_CONTEXT eventContext;
	eventContext = AllocateEventContextRaw(EventContextLength);
	if (eventContext == NULL) {
		return NULL;
	}
	SetCommonEventContext(Dcb, eventContext, Irp, Ccb);
	eventContext->SerialNumber = InterlockedIncrement((LONG *)&Dcb->SerialNumber);

	return eventContext;
}
