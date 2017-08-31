
#include "Stdafx.h"
#include "LockControl.h"

VOID DokanCompleteLock(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo) {
	PIRP irp;
	PIO_STACK_LOCATION irpSp;

	irp = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;

	DDbgPrint("==> DokanCompleteLock\n");

	DokanCompleteIrpRequest(irp, EventInfo->Status, 0);

	DDbgPrint("<== DokanCompleteLock\n");
}

NTSTATUS
DokanDispatchLock(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	PIO_STACK_LOCATION irpSp;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PFILE_OBJECT fileObject;
	PDokanCCB ccb;
	PDokanFCB fcb;
	PDokanVCB vcb;
	PDokanDCB dcb;
	PEVENT_CONTEXT eventContext;
	ULONG eventLength;
	BOOLEAN completeIrp = TRUE;

	__try {
		DDbgPrint("==> DokanLock\n");

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;

		if (fileObject == NULL) {
			DDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		vcb = (PDokanVCB)DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB ||
			!DokanCheckCCB(vcb->Dcb, (PDokanCCB)fileObject->FsContext2)) {
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		dcb = vcb->Dcb;

		DDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		DokanPrintFileName(fileObject);

		switch (irpSp->MinorFunction) {
		case IRP_MN_LOCK:
			DDbgPrint("  IRP_MN_LOCK\n");
			break;
		case IRP_MN_UNLOCK_ALL:
			DDbgPrint("  IRP_MN_UNLOCK_ALL\n");
			break;
		case IRP_MN_UNLOCK_ALL_BY_KEY:
			DDbgPrint("  IRP_MN_UNLOCK_ALL_BY_KEY\n");
			break;
		case IRP_MN_UNLOCK_SINGLE:
			DDbgPrint("  IRP_MN_UNLOCK_SINGLE\n");
			break;
		default:
			DDbgPrint("  unknown function : %d\n", irpSp->MinorFunction);
			break;
		}

		ccb = (PDokanCCB)fileObject->FsContext2;
		ASSERT(ccb != NULL);

		fcb = ccb->Fcb;
		ASSERT(fcb != NULL);

		eventLength = sizeof(EVENT_CONTEXT) + fcb->FileName.Length;
		eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);

		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		eventContext->Context = ccb->UserContext;
		DDbgPrint("   get Context %X\n", (ULONG)ccb->UserContext);

		// copy file name to be locked
		eventContext->Operation.Lock.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->Operation.Lock.FileName, fcb->FileName.Buffer,
			fcb->FileName.Length);

		// parameters of Lock
		eventContext->Operation.Lock.ByteOffset =
			irpSp->Parameters.LockControl.ByteOffset;
		if (irpSp->Parameters.LockControl.Length != NULL) {
			eventContext->Operation.Lock.Length.QuadPart =
				irpSp->Parameters.LockControl.Length->QuadPart;
		}
		else {
			DDbgPrint("  LockControl.Length = NULL\n");
		}
		eventContext->Operation.Lock.Key = irpSp->Parameters.LockControl.Key;

		if (dcb->FileLockInUserMode) {
			// register this IRP to waiting IRP list and make it pending status
			status = DokanRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);
		}
		else {
			status = DokanCommonLockControl(Irp, eventContext);
			completeIrp = FALSE;
		}

	}
	__finally {

		if (completeIrp) {
			DokanCompleteIrpRequest(Irp, status, 0);
		}

		DDbgPrint("<== DokanLock\n");
	}

	return status;
}

NTSTATUS
DokanCommonLockControl(__in PIRP Irp, __in PEVENT_CONTEXT EventContext) {
	NTSTATUS Status = STATUS_SUCCESS;

	PDokanFCB Fcb;
	PDokanCCB Ccb;
	PFILE_OBJECT fileObject;

	BOOLEAN AcquiredFcb = FALSE;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

	DDbgPrint("==> DokanCommonLockControl\n");

	PAGED_CODE();

	fileObject = irpSp->FileObject;
	DokanPrintFileName(fileObject);

	Ccb = (PDokanCCB)fileObject->FsContext2;
	if (Ccb == NULL || Ccb->Identifier.Type != CCB) {
		DDbgPrint("    DokanOplockRequest STATUS_INVALID_PARAMETER\n");
		return STATUS_INVALID_PARAMETER;
	}

	Fcb = Ccb->Fcb;
	if (Fcb == NULL || Fcb->Identifier.Type != FCB) {
		DDbgPrint("    DokanOplockRequest STATUS_INVALID_PARAMETER\n");
		return STATUS_INVALID_PARAMETER;
	}

	//
	//  If the file is not a user file open then we reject the request
	//  as an invalid parameter
	//
	if (FlagOn(Fcb->Flags, DOKAN_FILE_DIRECTORY)) {
		DDbgPrint("  DokanCommonLockControl -> STATUS_INVALID_PARAMETER\n", 0);
		return STATUS_INVALID_PARAMETER;
	}

	//
	//  Acquire exclusive access to the Fcb
	//
	AcquiredFcb = ExAcquireResourceSharedLite(&Fcb->Resource, TRUE);

	__try {

		//
		//  We check whether we can proceed
		//  based on the state of the file oplocks.
		//
#if (NTDDI_VERSION >= NTDDI_WIN8)

		if (((IRP_MN_LOCK == irpSp->MinorFunction) &&
			((ULONGLONG)irpSp->Parameters.LockControl.ByteOffset.QuadPart <
			(ULONGLONG)Fcb->AdvancedFCBHeader.AllocationSize.QuadPart)) ||
				((IRP_MN_LOCK != irpSp->MinorFunction) &&
					FsRtlAreThereWaitingFileLocks(&Fcb->FileLock))) {

			//
			//  Check whether we can proceed based on the state of file oplocks if doing
			//  an operation that interferes with oplocks. Those operations are:
			//
			//      1. Lock a range within the file's AllocationSize.
			//      2. Unlock a range when there are waiting locks on the file. This one
			//         is not guaranteed to interfere with oplocks, but it could, as
			//         unlocking this range might cause a waiting lock to be granted
			//         within AllocationSize!
			//
#endif
			Status = FsRtlCheckOplock(DokanGetFcbOplock(Fcb), Irp, EventContext,
				DokanOplockComplete, NULL);

#if (NTDDI_VERSION >= NTDDI_WIN8)
		}
#endif

		if (Status != STATUS_SUCCESS) {
			__leave;
		}

		//
		//  Now call the FsRtl routine to do the actual processing of the
		//  Lock request
		//
		Status = FsRtlProcessFileLock(&Fcb->FileLock, Irp, NULL);
	}
	__finally {
		//
		//  Release the Fcb, and return to our caller
		//
		if (AcquiredFcb) {
			ExReleaseResourceLite(&Fcb->Resource);
		}
	}

	DDbgPrint("<== DokanCommonLockControl\n");

	return Status;
}
