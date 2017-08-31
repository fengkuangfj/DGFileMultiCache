
#include "Stdafx.h"
#include "Mount.h"

VOID DokanUnmount(__in PDokanDCB Dcb) {
	ULONG eventLength;
	PEVENT_CONTEXT eventContext;
	PDRIVER_EVENT_CONTEXT driverEventContext;
	PKEVENT completedEvent;
	LARGE_INTEGER timeout;
	PDokanVCB vcb = (PDokanVCB)Dcb->Vcb;
	ULONG deviceNamePos;

	DDbgPrint("==> DokanUnmount\n");

	eventLength = sizeof(EVENT_CONTEXT);
	eventContext = AllocateEventContextRaw(eventLength);

	if (eventContext == NULL) {
		; // STATUS_INSUFFICIENT_RESOURCES;
		DDbgPrint(" Not able to allocate eventContext.\n");
		if (vcb) {
			DokanEventRelease(vcb->DeviceObject);
		}
		return;
	}

	driverEventContext =
		CONTAINING_RECORD(eventContext, DRIVER_EVENT_CONTEXT, EventContext);
	completedEvent = (PKEVENT)ExAllocatePool(sizeof(KEVENT));
	if (completedEvent) {
		KeInitializeEvent(completedEvent, NotificationEvent, FALSE);
		driverEventContext->Completed = completedEvent;
	}

	deviceNamePos = Dcb->SymbolicLinkName->Length / sizeof(WCHAR) - 1;
	for (; Dcb->SymbolicLinkName->Buffer[deviceNamePos] != L'\\'; --deviceNamePos)
		;
	RtlStringCchCopyW(eventContext->Operation.Unmount.DeviceName,
		sizeof(eventContext->Operation.Unmount.DeviceName) /
		sizeof(WCHAR),
		&(Dcb->SymbolicLinkName->Buffer[deviceNamePos]));

	DDbgPrint("  Send Unmount to Service : %ws\n",
		eventContext->Operation.Unmount.DeviceName);

	DokanEventNotification(&Dcb->Global->NotifyService, eventContext);

	if (completedEvent) {
		timeout.QuadPart = -1 * 10 * 1000 * 10; // 10 sec
		KeWaitForSingleObject(completedEvent, Executive, KernelMode, FALSE,
			&timeout);
	}

	if (vcb) {
		DokanEventRelease(vcb->DeviceObject);
	}

	if (completedEvent) {
		ExFreePool(completedEvent);
	}

	DDbgPrint("<== DokanUnmount\n");
}

NTSTATUS IsMountPointDriveLetter(__in PUNICODE_STRING mountPoint) {
	if (mountPoint != NULL) {
		// Check if mount point match \DosDevices\C:
		USHORT length = mountPoint->Length / sizeof(WCHAR);
		if (length > 12 && length <= 15) {
			return STATUS_SUCCESS;
		}
	}

	return STATUS_INVALID_PARAMETER;
}

VOID DokanDeleteMountPoint(__in PDokanDCB Dcb) {
	NTSTATUS status;

	if (Dcb->MountPoint != NULL && Dcb->MountPoint->Length > 0) {
		if (Dcb->UseMountManager) {
			Dcb->UseMountManager = FALSE; // To avoid recursive call
			DokanSendVolumeDeletePoints(Dcb->MountPoint, Dcb->DiskDeviceName);
		}
		else {
			if (Dcb->MountGlobally) {
				// Run DokanDeleteMountPointProc in System thread.
				HANDLE handle;
				PKTHREAD thread;
				OBJECT_ATTRIBUTES objectAttribs;

				InitializeObjectAttributes(&objectAttribs, NULL, OBJ_KERNEL_HANDLE,
					NULL, NULL);
				status = PsCreateSystemThread(
					&handle, THREAD_ALL_ACCESS, &objectAttribs, NULL, NULL,
					(PKSTART_ROUTINE)DokanDeleteMountPointSysProc, Dcb);
				if (!NT_SUCCESS(status)) {
					DDbgPrint("DokanDeleteMountPoint PsCreateSystemThread failed: 0x%X\n",
						status);
				}
				else {
					ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, NULL, KernelMode,
						(PVOID *)&thread, NULL);
					ZwClose(handle);
					KeWaitForSingleObject(thread, Executive, KernelMode, FALSE, NULL);
					ObDereferenceObject(thread);
				}
			}
			else {
				DokanDeleteMountPointSysProc(Dcb);
			}
		}
	}
}

VOID DokanCreateMountPoint(__in PDokanDCB Dcb) {
	NTSTATUS status;

	if (Dcb->MountPoint != NULL && Dcb->MountPoint->Length > 0) {
		if (Dcb->UseMountManager) {
			DokanSendVolumeCreatePoint(Dcb->DiskDeviceName, Dcb->MountPoint);
		}
		else {
			if (Dcb->MountGlobally) {
				// Run DokanCreateMountPointProc in system thread.
				HANDLE handle;
				PKTHREAD thread;
				OBJECT_ATTRIBUTES objectAttribs;

				InitializeObjectAttributes(&objectAttribs, NULL, OBJ_KERNEL_HANDLE,
					NULL, NULL);
				status = PsCreateSystemThread(
					&handle, THREAD_ALL_ACCESS, &objectAttribs, NULL, NULL,
					(PKSTART_ROUTINE)DokanCreateMountPointSysProc, Dcb);
				if (!NT_SUCCESS(status)) {
					DDbgPrint("DokanCreateMountPoint PsCreateSystemThread failed: 0x%X\n",
						status);
				}
				else {
					ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, NULL, KernelMode,
						(PVOID *)&thread, NULL);
					ZwClose(handle);
					KeWaitForSingleObject(thread, Executive, KernelMode, FALSE, NULL);
					ObDereferenceObject(thread);
				}
			}
			else {
				DokanCreateMountPointSysProc(Dcb);
			}
		}
	}
}

NTSTATUS
DokanGetMountPointList(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in PDOKAN_GLOBAL dokanGlobal) {
	UNREFERENCED_PARAMETER(DeviceObject);
	PIO_STACK_LOCATION irpSp = NULL;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PLIST_ENTRY listEntry;
	PMOUNT_ENTRY mountEntry;
	PDOKAN_CONTROL dokanControl;
	int i = 0;

	DDbgPrint("==> DokanGetMountPointList\n");
	irpSp = IoGetCurrentIrpStackLocation(Irp);

	__try {
		dokanControl = (PDOKAN_CONTROL)Irp->AssociatedIrp.SystemBuffer;
		for (listEntry = dokanGlobal->MountPointList.Flink;
			listEntry != &dokanGlobal->MountPointList;
			listEntry = listEntry->Flink, ++i) {
			Irp->IoStatus.Information = sizeof(DOKAN_CONTROL) * (i + 1);
			if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
				(sizeof(DOKAN_CONTROL) * (i + 1))) {
				status = STATUS_BUFFER_OVERFLOW;
				__leave;
			}

			mountEntry = CONTAINING_RECORD(listEntry, MOUNT_ENTRY, ListEntry);
			RtlCopyMemory(&dokanControl[i], &mountEntry->MountControl,
				sizeof(DOKAN_CONTROL));
		}

		status = STATUS_SUCCESS;
	}
	__finally {
	}

	DDbgPrint("<== DokanGetMountPointList\n");
	return status;
}

PMOUNT_ENTRY
FindMountEntry(__in PDOKAN_GLOBAL dokanGlobal,
	__out PDOKAN_CONTROL DokanControl) {
	PLIST_ENTRY listEntry;
	PMOUNT_ENTRY mountEntry = NULL;
	BOOLEAN useMountPoint = (DokanControl->MountPoint[0] != L'\0');
	BOOLEAN found = FALSE;

	ExAcquireResourceExclusiveLite(&dokanGlobal->Resource, TRUE);

	for (listEntry = dokanGlobal->MountPointList.Flink;
		listEntry != &dokanGlobal->MountPointList;
		listEntry = listEntry->Flink) {
		mountEntry = CONTAINING_RECORD(listEntry, MOUNT_ENTRY, ListEntry);
		if (useMountPoint) {
			if (wcscmp(DokanControl->MountPoint,
				mountEntry->MountControl.MountPoint) == 0) {
				found = TRUE;
				break;
			}
		}
		else {
			if (wcscmp(DokanControl->DeviceName,
				mountEntry->MountControl.DeviceName) == 0) {
				found = TRUE;
				break;
			}
		}
	}

	ExReleaseResourceLite(&dokanGlobal->Resource);

	if (found) {
		DDbgPrint("FindMountEntry %ws -> %ws\n",
			mountEntry->MountControl.MountPoint,
			mountEntry->MountControl.DeviceName);
		return mountEntry;
	}
	else {
		return NULL;
	}
}

NTSTATUS
DokanSendIoContlToMountManager(__in ULONG IoControlCode, __in PVOID InputBuffer,
	__in ULONG Length, __out PVOID OutputBuffer,
	__in ULONG OutputLength) {
	NTSTATUS status;
	UNICODE_STRING mountManagerName;
	PFILE_OBJECT mountFileObject;
	PDEVICE_OBJECT mountDeviceObject;
	PIRP irp;
	KEVENT driverEvent;
	IO_STATUS_BLOCK iosb;

	DDbgPrint("=> DokanSendIoContlToMountManager\n");

	RtlInitUnicodeString(&mountManagerName, MOUNTMGR_DEVICE_NAME);

	status = IoGetDeviceObjectPointer(&mountManagerName, FILE_READ_ATTRIBUTES,
		&mountFileObject, &mountDeviceObject);

	if (!NT_SUCCESS(status)) {
		DDbgPrint("  IoGetDeviceObjectPointer failed: 0x%x\n", status);
		return status;
	}

	KeInitializeEvent(&driverEvent, NotificationEvent, FALSE);

	irp = IoBuildDeviceIoControlRequest(IoControlCode, mountDeviceObject,
		InputBuffer, Length, OutputBuffer,
		OutputLength, FALSE, &driverEvent, &iosb);

	if (irp == NULL) {
		DDbgPrint("  IoBuildDeviceIoControlRequest failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	status = IoCallDriver(mountDeviceObject, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&driverEvent, Executive, KernelMode, FALSE, NULL);
	}
	status = iosb.Status;

	ObDereferenceObject(mountFileObject);
	// Don't dereference mountDeviceObject, mountFileObject is enough

	if (NT_SUCCESS(status)) {
		DDbgPrint("  IoCallDriver success\n");
	}
	else {
		DDbgPrint("  IoCallDriver failed: 0x%x\n", status);
	}

	DDbgPrint("<= DokanSendIoContlToMountManager\n");

	return status;
}

NTSTATUS DokanMountVolume(__in PDEVICE_OBJECT DiskDevice, __in PIRP Irp) {
	PDokanDCB dcb = NULL;
	PDokanVCB vcb = NULL;
	PVPB vpb = NULL;
	DOKAN_CONTROL dokanControl;
	PMOUNT_ENTRY mountEntry = NULL;
	PIO_STACK_LOCATION irpSp;
	PDEVICE_OBJECT volDeviceObject;
	PDRIVER_OBJECT DriverObject = DiskDevice->DriverObject;
	NTSTATUS status = STATUS_UNRECOGNIZED_VOLUME;

	irpSp = IoGetCurrentIrpStackLocation(Irp);
	dcb = (PDokanDCB)irpSp->Parameters.MountVolume.DeviceObject->DeviceExtension;
	if (!dcb) {
		DDbgPrint("   Not DokanDiskDevice (no device extension)\n");
		return status;
	}
	PrintIdType(dcb);
	if (GetIdentifierType(dcb) != DCB) {
		DDbgPrint("   Not DokanDiskDevice\n");
		return status;
	}
	BOOLEAN isNetworkFileSystem =
		(dcb->VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM);

	if (!isNetworkFileSystem) {
		status = IoCreateDevice(DriverObject,               // DriverObject
			sizeof(DokanVCB),           // DeviceExtensionSize
			NULL,                       // DeviceName
			dcb->VolumeDeviceType,      // DeviceType
			dcb->DeviceCharacteristics, // DeviceCharacteristics
			FALSE,                      // Not Exclusive
			&volDeviceObject);          // DeviceObject
	}
	else {
		status = IoCreateDeviceSecure(
			DriverObject,               // DriverObject
			sizeof(DokanVCB),           // DeviceExtensionSize
			dcb->DiskDeviceName,        // DeviceName
			dcb->VolumeDeviceType,      // DeviceType
			dcb->DeviceCharacteristics, // DeviceCharacteristics
			FALSE,                      // Not Exclusive
			&sddl,                      // Default SDDL String
			NULL,                       // Device Class GUID
			&volDeviceObject);          // DeviceObject
	}

	if (!NT_SUCCESS(status)) {
		DDbgPrint("  IoCreateDevice failed: 0x%x\n", status);
		return status;
	}

	vcb = (PDokanVCB)volDeviceObject->DeviceExtension;
	vcb->Identifier.Type = VCB;
	vcb->Identifier.Size = sizeof(DokanVCB);

	vcb->DeviceObject = volDeviceObject;
	vcb->Dcb = dcb;
	dcb->Vcb = vcb;

	InitializeListHead(&vcb->NextFCB);

	InitializeListHead(&vcb->DirNotifyList);
	FsRtlNotifyInitializeSync(&vcb->NotifySync);

	ExInitializeFastMutex(&vcb->AdvancedFCBHeaderMutex);

#if _WIN32_WINNT >= 0x0501
	FsRtlSetupAdvancedHeader(&vcb->VolumeFileHeader,
		&vcb->AdvancedFCBHeaderMutex);
#else
	if (DokanFsRtlTeardownPerStreamContexts) {
		FsRtlSetupAdvancedHeader(&vcb->VolumeFileHeader,
			&vcb->AdvancedFCBHeaderMutex);
	}
#endif

	vpb = irpSp->Parameters.MountVolume.Vpb;
	DokanInitVpb(vpb, vcb->DeviceObject);

	//
	// Establish user-buffer access method.
	//
	volDeviceObject->Flags |= DO_DIRECT_IO;

	volDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	dcb->Mounted = 1;
	ObReferenceObject(volDeviceObject);

	// set the device on dokanControl
	RtlZeroMemory(&dokanControl, sizeof(dokanControl));
	RtlCopyMemory(dokanControl.DeviceName, dcb->DiskDeviceName->Buffer,
		dcb->DiskDeviceName->Length);
	if (dcb->UNCName->Buffer != NULL && dcb->UNCName->Length > 0) {
		RtlCopyMemory(dokanControl.UNCName, dcb->UNCName->Buffer,
			dcb->UNCName->Length);
	}
	mountEntry = FindMountEntry(dcb->Global, &dokanControl);
	if (mountEntry != NULL) {
		mountEntry->MountControl.DeviceObject = volDeviceObject;
	}
	else {
		DDbgPrint("MountEntry not found. This way the dokanControl does not have "
			"the DeviceObject")
	}

	// Start check thread
	ExAcquireResourceExclusiveLite(&dcb->Resource, TRUE);
	DokanUpdateTimeout(&dcb->TickCount, DOKAN_KEEPALIVE_TIMEOUT * 3);
	ExReleaseResourceLite(&dcb->Resource);
	DokanStartCheckThread(dcb);

	// Create mount point for the volume
	if (dcb->UseMountManager) {
		status = DokanSendVolumeArrivalNotification(dcb->DiskDeviceName);
		if (!NT_SUCCESS(status)) {
			DDbgPrint("  DokanSendVolumeArrivalNotification failed: 0x%x\n", status);
		}
	}
	DokanCreateMountPoint(dcb);

	if (isNetworkFileSystem) {
		DokanRegisterUncProviderSystem(dcb);
	}
	return STATUS_SUCCESS;
}

PMOUNT_ENTRY
InsertMountEntry(PDOKAN_GLOBAL dokanGlobal, PDOKAN_CONTROL DokanControl) {
	PMOUNT_ENTRY mountEntry;
	mountEntry = (PMOUNT_ENTRY)ExAllocatePool(sizeof(MOUNT_ENTRY));
	if (mountEntry == NULL) {
		DDbgPrint("  InsertMountEntry allocation failed\n");
		return NULL;
	}
	RtlZeroMemory(mountEntry, sizeof(MOUNT_ENTRY));
	RtlCopyMemory(&mountEntry->MountControl, DokanControl, sizeof(DOKAN_CONTROL));

	InitializeListHead(&mountEntry->ListEntry);

	ExAcquireResourceExclusiveLite(&dokanGlobal->Resource, TRUE);
	InsertTailList(&dokanGlobal->MountPointList, &mountEntry->ListEntry);
	ExReleaseResourceLite(&dokanGlobal->Resource);

	return mountEntry;
}

VOID RemoveMountEntry(PDOKAN_GLOBAL dokanGlobal, PMOUNT_ENTRY MountEntry) {
	ExAcquireResourceExclusiveLite(&dokanGlobal->Resource, TRUE);
	RemoveEntryList(&MountEntry->ListEntry);
	ExReleaseResourceLite(&dokanGlobal->Resource);

	ExFreePool(MountEntry);
}

VOID DokanCreateMountPointSysProc(__in PDokanDCB Dcb) {
	NTSTATUS status;

	DDbgPrint("=> DokanCreateMountPointSysProc\n");

	status = IoCreateSymbolicLink(Dcb->MountPoint, Dcb->DiskDeviceName);
	if (!NT_SUCCESS(status)) {
		DDbgPrint("IoCreateSymbolicLink for mount point %wZ failed: 0x%X\n",
			Dcb->MountPoint, status);
	}

	DDbgPrint("<= DokanCreateMountPointSysProc\n");
}

VOID DokanDeleteMountPointSysProc(__in PDokanDCB Dcb) {
	DDbgPrint("=> DokanDeleteMountPointSysProc\n");
	if (Dcb->MountPoint != NULL && Dcb->MountPoint->Length > 0) {
		DDbgPrint("  Delete Mount Point Symbolic Name: %wZ\n", Dcb->MountPoint);
		IoDeleteSymbolicLink(Dcb->MountPoint);
	}
	DDbgPrint("<= DokanDeleteMountPointSysProc\n");
}
