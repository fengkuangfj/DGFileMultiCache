
#include "Stdafx.h"
#include "Irp.h"

VOID DokanIrpCancelRoutine(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	KIRQL oldIrql;
	PIRP_ENTRY irpEntry;
	ULONG serialNumber = 0;
	PIO_STACK_LOCATION irpSp;

	UNREFERENCED_PARAMETER(DeviceObject);

	DDbgPrint("==> DokanIrpCancelRoutine\n");

	// Release the cancel spinlock
	IoReleaseCancelSpinLock(Irp->CancelIrql);

	irpEntry = (PIRP_ENTRY)Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_IRP_ENTRY];

	if (irpEntry != NULL) {
		PKSPIN_LOCK lock = &irpEntry->IrpList->ListLock;

		// Acquire the queue spinlock
		ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
		KeAcquireSpinLock(lock, &oldIrql);

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		ASSERT(irpSp != NULL);

		serialNumber = irpEntry->SerialNumber;

		RemoveEntryList(&irpEntry->ListEntry);

		// If Write is canceld before completion and buffer that saves writing
		// content is not freed, free it here
		if (irpSp->MajorFunction == IRP_MJ_WRITE) {
			PVOID eventContext =
				Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_EVENT];
			if (eventContext != NULL) {
				DokanFreeEventContext((PEVENT_CONTEXT)eventContext);
			}
			Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_EVENT] = NULL;
		}

		if (IsListEmpty(&irpEntry->IrpList->ListHead)) {
			// DDbgPrint("    list is empty ClearEvent\n");
			KeClearEvent(&irpEntry->IrpList->NotEmpty);
		}

		irpEntry->Irp = NULL;

		if (irpEntry->CancelRoutineFreeMemory == FALSE) {
			InitializeListHead(&irpEntry->ListEntry);
		}
		else {
			DokanFreeIrpEntry(irpEntry);
			irpEntry = NULL;
		}

		Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_IRP_ENTRY] = NULL;

		KeReleaseSpinLock(lock, oldIrql);
	}

	DDbgPrint("   canceled IRP #%X\n", serialNumber);
	DokanCompleteIrpRequest(Irp, STATUS_CANCELLED, 0);

	DDbgPrint("<== DokanIrpCancelRoutine\n");
	return;
}

VOID DokanPrePostIrp(IN PVOID Context, IN PIRP Irp)
/*++
Routine Description:
This routine performs any neccessary work before STATUS_PENDING is
returned with the Fsd thread.  This routine is called within the
filesystem and by the oplock package.
Arguments:
Context - Pointer to the EventContext to be queued to the Fsp
Irp - I/O Request Packet.
Return Value:
None.
--*/
{
	DDbgPrint("==> DokanPrePostIrp\n");

	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(Irp);

	DDbgPrint("<== DokanPrePostIrp\n");
}

NTSTATUS
RegisterPendingIrpMain(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in ULONG SerialNumber, __in PIRP_LIST IrpList,
	__in ULONG Flags, __in ULONG CheckMount) {
	PIRP_ENTRY irpEntry;
	PIO_STACK_LOCATION irpSp;
	KIRQL oldIrql;
	PDokanVCB vcb = NULL;

	DDbgPrint("==> DokanRegisterPendingIrpMain\n");

	if (GetIdentifierType(DeviceObject->DeviceExtension) == VCB) {
		vcb = (PDokanVCB)DeviceObject->DeviceExtension;
		if (CheckMount && !vcb->Dcb->Mounted) {
			DDbgPrint(" device is not mounted\n");
			return STATUS_DEVICE_DOES_NOT_EXIST;
		}
	}

	irpSp = IoGetCurrentIrpStackLocation(Irp);

	// Allocate a record and save all the event context.
	irpEntry = (PIRP_ENTRY)DokanAllocateIrpEntry();

	if (NULL == irpEntry) {
		DDbgPrint("  can't allocate IRP_ENTRY\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(irpEntry, sizeof(IRP_ENTRY));

	InitializeListHead(&irpEntry->ListEntry);

	irpEntry->SerialNumber = SerialNumber;
	irpEntry->FileObject = irpSp->FileObject;
	irpEntry->Irp = Irp;
	irpEntry->IrpSp = irpSp;
	irpEntry->IrpList = IrpList;
	irpEntry->Flags = Flags;

	// Update the irp timeout for the entry
	if (vcb) {
		ExAcquireResourceExclusiveLite(&vcb->Dcb->Resource, TRUE);
		DokanUpdateTimeout(&irpEntry->TickCount, vcb->Dcb->IrpTimeout);
		ExReleaseResourceLite(&vcb->Dcb->Resource);
	}
	else {
		DokanUpdateTimeout(&irpEntry->TickCount, DOKAN_IRP_PENDING_TIMEOUT);
	}

	// DDbgPrint("  Lock IrpList.ListLock\n");
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeAcquireSpinLock(&IrpList->ListLock, &oldIrql);

	IoSetCancelRoutine(Irp, DokanIrpCancelRoutine);

	if (Irp->Cancel) {
		if (IoSetCancelRoutine(Irp, NULL) != NULL) {
			// DDbgPrint("  Release IrpList.ListLock %d\n", __LINE__);
			KeReleaseSpinLock(&IrpList->ListLock, oldIrql);

			DokanFreeIrpEntry(irpEntry);

			return STATUS_CANCELLED;
		}
	}

	IoMarkIrpPending(Irp);

	InsertTailList(&IrpList->ListHead, &irpEntry->ListEntry);

	irpEntry->CancelRoutineFreeMemory = FALSE;

	// save the pointer in order to be accessed by cancel routine
	Irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_IRP_ENTRY] = irpEntry;

	KeSetEvent(&IrpList->NotEmpty, IO_NO_INCREMENT, FALSE);

	// DDbgPrint("  Release IrpList.ListLock\n");
	KeReleaseSpinLock(&IrpList->ListLock, oldIrql);

	DDbgPrint("<== DokanRegisterPendingIrpMain\n");
	return STATUS_PENDING;
}

NTSTATUS
DokanRegisterPendingIrp(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in PEVENT_CONTEXT EventContext, __in ULONG Flags) {
	PDokanVCB vcb = (PDokanVCB)DeviceObject->DeviceExtension;
	NTSTATUS status;

	DDbgPrint("==> DokanRegisterPendingIrp\n");

	if (GetIdentifierType(vcb) != VCB) {
		DDbgPrint("  IdentifierType is not VCB\n");
		return STATUS_INVALID_PARAMETER;
	}

	status = RegisterPendingIrpMain(DeviceObject, Irp, EventContext->SerialNumber,
		&vcb->Dcb->PendingIrp, Flags, TRUE);

	if (status == STATUS_PENDING) {
		DokanEventNotification(&vcb->Dcb->NotifyEvent, EventContext);
	}
	else {
		DokanFreeEventContext(EventContext);
	}

	DDbgPrint("<== DokanRegisterPendingIrp\n");
	return status;
}

NTSTATUS
DokanRegisterPendingIrpForEvent(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp) {
	PDokanVCB vcb = (PDokanVCB)DeviceObject->DeviceExtension;

	if (GetIdentifierType(vcb) != VCB) {
		DDbgPrint("  IdentifierType is not VCB\n");
		return STATUS_INVALID_PARAMETER;
	}

	// DDbgPrint("DokanRegisterPendingIrpForEvent\n");
	vcb->HasEventWait = TRUE;

	return RegisterPendingIrpMain(DeviceObject, Irp,
		0, // SerialNumber
		&vcb->Dcb->PendingEvent,
		0, // Flags
		TRUE);
}

NTSTATUS
DokanRegisterPendingIrpForService(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp) {
	PDOKAN_GLOBAL dokanGlobal;
	DDbgPrint("DokanRegisterPendingIrpForService\n");

	dokanGlobal = (PDOKAN_GLOBAL)DeviceObject->DeviceExtension;
	if (GetIdentifierType(dokanGlobal) != DGL) {
		return STATUS_INVALID_PARAMETER;
	}

	return RegisterPendingIrpMain(DeviceObject, Irp,
		0, // SerialNumber
		&dokanGlobal->PendingService,
		0, // Flags
		FALSE);
}

// When user-mode file system application returns EventInformation,
// search corresponding pending IRP and complete it
NTSTATUS
DokanCompleteIrp(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	KIRQL oldIrql;
	PLIST_ENTRY thisEntry, nextEntry, listHead;
	PIRP_ENTRY irpEntry;
	PDokanVCB vcb;
	PEVENT_INFORMATION eventInfo;

	eventInfo = (PEVENT_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
	ASSERT(eventInfo != NULL);

	// DDbgPrint("==> DokanCompleteIrp [EventInfo #%X]\n",
	// eventInfo->SerialNumber);

	vcb = (PDokanVCB)DeviceObject->DeviceExtension;
	if (GetIdentifierType(vcb) != VCB) {
		return STATUS_INVALID_PARAMETER;
	}

	// DDbgPrint("      Lock IrpList.ListLock\n");
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeAcquireSpinLock(&vcb->Dcb->PendingIrp.ListLock, &oldIrql);

	// search corresponding IRP through pending IRP list
	listHead = &vcb->Dcb->PendingIrp.ListHead;

	for (thisEntry = listHead->Flink; thisEntry != listHead;
		thisEntry = nextEntry) {

		PIRP irp;
		PIO_STACK_LOCATION irpSp;

		nextEntry = thisEntry->Flink;

		irpEntry = CONTAINING_RECORD(thisEntry, IRP_ENTRY, ListEntry);

		// check whether this is corresponding IRP

		// DDbgPrint("SerialNumber irpEntry %X eventInfo %X\n",
		// irpEntry->SerialNumber, eventInfo->SerialNumber);

		// this irpEntry must be freed in this if statement
		if (irpEntry->SerialNumber != eventInfo->SerialNumber) {
			continue;
		}

		RemoveEntryList(thisEntry);

		irp = irpEntry->Irp;

		if (irp == NULL) {
			// this IRP is already canceled
			ASSERT(irpEntry->CancelRoutineFreeMemory == FALSE);
			DokanFreeIrpEntry(irpEntry);
			irpEntry = NULL;
			break;
		}

		if (IoSetCancelRoutine(irp, NULL) == NULL) {
			// Cancel routine will run as soon as we release the lock
			InitializeListHead(&irpEntry->ListEntry);
			irpEntry->CancelRoutineFreeMemory = TRUE;
			break;
		}

		// IRP is not canceled yet
		irpSp = irpEntry->IrpSp;

		ASSERT(irpSp != NULL);

		// IrpEntry is saved here for CancelRoutine
		// Clear it to prevent to be completed by CancelRoutine twice
		irp->Tail.Overlay.DriverContext[DRIVER_CONTEXT_IRP_ENTRY] = NULL;
		KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);

		switch (irpSp->MajorFunction) {
		case IRP_MJ_DIRECTORY_CONTROL:
			DokanCompleteDirectoryControl(irpEntry, eventInfo);
			break;
		case IRP_MJ_READ:
			DokanCompleteRead(irpEntry, eventInfo);
			break;
		case IRP_MJ_WRITE:
			DokanCompleteWrite(irpEntry, eventInfo);
			break;
		case IRP_MJ_QUERY_INFORMATION:
			DokanCompleteQueryInformation(irpEntry, eventInfo);
			break;
		case IRP_MJ_QUERY_VOLUME_INFORMATION:
			DokanCompleteQueryVolumeInformation(irpEntry, eventInfo, DeviceObject);
			break;
		case IRP_MJ_CREATE:
			DokanCompleteCreate(irpEntry, eventInfo);
			break;
		case IRP_MJ_CLEANUP:
			DokanCompleteCleanup(irpEntry, eventInfo);
			break;
		case IRP_MJ_LOCK_CONTROL:
			DokanCompleteLock(irpEntry, eventInfo);
			break;
		case IRP_MJ_SET_INFORMATION:
			DokanCompleteSetInformation(irpEntry, eventInfo);
			break;
		case IRP_MJ_FLUSH_BUFFERS:
			DokanCompleteFlush(irpEntry, eventInfo);
			break;
		case IRP_MJ_QUERY_SECURITY:
			DokanCompleteQuerySecurity(irpEntry, eventInfo);
			break;
		case IRP_MJ_SET_SECURITY:
			DokanCompleteSetSecurity(irpEntry, eventInfo);
			break;
		default:
			DDbgPrint("Unknown IRP %d\n", irpSp->MajorFunction);
			// TODO: in this case, should complete this IRP
			break;
		}

		DokanFreeIrpEntry(irpEntry);
		irpEntry = NULL;

		return STATUS_SUCCESS;
	}

	KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);

	// DDbgPrint("<== AACompleteIrp [EventInfo #%X]\n", eventInfo->SerialNumber);

	// TODO: should return error
	return STATUS_SUCCESS;
}

VOID ReleasePendingIrp(__in PIRP_LIST PendingIrp) {
	PLIST_ENTRY listHead;
	LIST_ENTRY completeList;
	PIRP_ENTRY irpEntry;
	KIRQL oldIrql;
	PIRP irp;

	InitializeListHead(&completeList);

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeAcquireSpinLock(&PendingIrp->ListLock, &oldIrql);

	while (!IsListEmpty(&PendingIrp->ListHead)) {
		listHead = RemoveHeadList(&PendingIrp->ListHead);
		irpEntry = CONTAINING_RECORD(listHead, IRP_ENTRY, ListEntry);
		irp = irpEntry->Irp;
		if (irp == NULL) {
			// this IRP has already been canceled
			ASSERT(irpEntry->CancelRoutineFreeMemory == FALSE);
			DokanFreeIrpEntry(irpEntry);
			continue;
		}

		if (IoSetCancelRoutine(irp, NULL) == NULL) {
			// Cancel routine will run as soon as we release the lock
			InitializeListHead(&irpEntry->ListEntry);
			irpEntry->CancelRoutineFreeMemory = TRUE;
			continue;
		}
		InsertTailList(&completeList, &irpEntry->ListEntry);
	}

	KeClearEvent(&PendingIrp->NotEmpty);
	KeReleaseSpinLock(&PendingIrp->ListLock, oldIrql);

	while (!IsListEmpty(&completeList)) {
		listHead = RemoveHeadList(&completeList);
		irpEntry = CONTAINING_RECORD(listHead, IRP_ENTRY, ListEntry);
		irp = irpEntry->Irp;
		DokanFreeIrpEntry(irpEntry);
		DokanCompleteIrpRequest(irp, STATUS_SUCCESS, 0);
	}
}

VOID DokanInitIrpList(__in PIRP_LIST IrpList) {
	InitializeListHead(&IrpList->ListHead);
	KeInitializeSpinLock(&IrpList->ListLock);
	KeInitializeEvent(&IrpList->NotEmpty, NotificationEvent, FALSE);
}

VOID DokanCompleteIrpRequest(__in PIRP Irp, __in NTSTATUS Status,
	__in ULONG_PTR Info) {
	if (Irp == NULL) {
		DDbgPrint("  Irp is NULL, so no complete required\n");
		return;
	}
	if (Status == -1) {
		DDbgPrint("  Status is -1 which is not valid NTSTATUS\n");
		Status = STATUS_INVALID_PARAMETER;
	}
	if (Status != STATUS_PENDING) {
		Irp->IoStatus.Status = Status;
		Irp->IoStatus.Information = Info;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	DokanPrintNTStatus(Status);
}
