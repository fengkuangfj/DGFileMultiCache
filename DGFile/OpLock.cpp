
#include "Stdafx.h"
#include "OpLock.h"

VOID DokanOplockComplete(IN PVOID Context, IN PIRP Irp)
/*++
Routine Description:
This routine is called by the oplock package when an oplock break has
completed, allowing an Irp to resume execution.  If the status in
the Irp is STATUS_SUCCESS, then we queue the Irp to the Fsp queue.
Otherwise we complete the Irp with the status in the Irp.
Arguments:
Context - Pointer to the EventContext to be queued to the Fsp
Irp - I/O Request Packet.
Return Value:
None.
--*/
{
	PIO_STACK_LOCATION irpSp;

	DDbgPrint("==> DokanOplockComplete\n");
	PAGED_CODE();

	irpSp = IoGetCurrentIrpStackLocation(Irp);

	//
	//  Check on the return value in the Irp.
	//
	if (Irp->IoStatus.Status == STATUS_SUCCESS) {
		DokanRegisterPendingIrp(irpSp->DeviceObject, Irp, (PEVENT_CONTEXT)Context,
			0);
	}
	else {
		DokanCompleteIrpRequest(Irp, Irp->IoStatus.Status, 0);
	}

	DDbgPrint("<== DokanOplockComplete\n");

	return;
}

NTSTATUS DokanOplockRequest(__in PIRP *pIrp) {
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG FsControlCode;
	PDokanDCB Dcb;
	PDokanVCB Vcb;
	PDokanFCB Fcb;
	PDokanCCB Ccb;
	PFILE_OBJECT fileObject;
	PIRP Irp = *pIrp;

	ULONG OplockCount = 0;

	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

	BOOLEAN AcquiredVcb = FALSE;
	BOOLEAN AcquiredFcb = FALSE;

#if (NTDDI_VERSION >= NTDDI_WIN7)
	PREQUEST_OPLOCK_INPUT_BUFFER InputBuffer = NULL;
	ULONG InputBufferLength;
	ULONG OutputBufferLength;
#endif

	PAGED_CODE();

	//
	//  Save some references to make our life a little easier
	//
	FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

	fileObject = IrpSp->FileObject;
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

	Vcb = Fcb->Vcb;
	if (Vcb == NULL || Vcb->Identifier.Type != VCB) {
		DDbgPrint("    DokanOplockRequest STATUS_INVALID_PARAMETER\n");
		return STATUS_INVALID_PARAMETER;
	}
	Dcb = Vcb->Dcb;

#if (NTDDI_VERSION >= NTDDI_WIN7)

	//
	//  Get the input & output buffer lengths and pointers.
	//
	if (FsControlCode == FSCTL_REQUEST_OPLOCK) {

		InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
		InputBuffer = (PREQUEST_OPLOCK_INPUT_BUFFER)Irp->AssociatedIrp.SystemBuffer;

		OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

		//
		//  Check for a minimum length on the input and ouput buffers.
		//
		if ((InputBufferLength < sizeof(REQUEST_OPLOCK_INPUT_BUFFER)) ||
			(OutputBufferLength < sizeof(REQUEST_OPLOCK_OUTPUT_BUFFER))) {
			DDbgPrint("    DokanOplockRequest STATUS_BUFFER_TOO_SMALL\n");
			return STATUS_BUFFER_TOO_SMALL;
		}
	}

	//
	//  If the oplock request is on a directory it must be for a Read or
	//  Read-Handle
	//  oplock only.
	//
	if (FlagOn(Fcb->Flags, DOKAN_FILE_DIRECTORY) &&
		((FsControlCode != FSCTL_REQUEST_OPLOCK) ||
			!FsRtlOplockIsSharedRequest(Irp))) {

		DDbgPrint("    DokanOplockRequest STATUS_INVALID_PARAMETER\n");
		return STATUS_INVALID_PARAMETER;
	}

#endif

	//
	//  Use a try finally to free the Fcb/Vcb
	//
	__try {

		//
		//  We grab the Fcb exclusively for oplock requests, shared for oplock
		//  break acknowledgement.
		//
		if ((FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
			(FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK) ||
			(FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK) ||
			(FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)
#if (NTDDI_VERSION >= NTDDI_WIN7)
			|| ((FsControlCode == FSCTL_REQUEST_OPLOCK) &&
				FlagOn(InputBuffer->Flags, REQUEST_OPLOCK_INPUT_FLAG_REQUEST))
#endif
			) {

			AcquiredVcb = ExAcquireResourceSharedLite(&(Fcb->Vcb->Resource), TRUE);
			AcquiredFcb = ExAcquireResourceExclusiveLite(&(Fcb->Resource), TRUE);

#if (NTDDI_VERSION >= NTDDI_WIN7)
			if (!Dcb->FileLockInUserMode && FsRtlOplockIsSharedRequest(Irp)) {
#else
			if (!Dcb->FileLockInUserMode &&
				FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {
#endif
				//
				//  Byte-range locks are only valid on files.
				//
				if (!FlagOn(Fcb->Flags, DOKAN_FILE_DIRECTORY)) {

					//
					//  Set OplockCount to nonzero if FsRtl denies access
					//  based on current byte-range lock state.
					//
#if (NTDDI_VERSION >= NTDDI_WIN8)
					OplockCount = (ULONG)!FsRtlCheckLockForOplockRequest(
						&Fcb->FileLock, &Fcb->AdvancedFCBHeader.AllocationSize);
#elif (NTDDI_VERSION >= NTDDI_WIN7)
					OplockCount =
						(ULONG)FsRtlAreThereCurrentOrInProgressFileLocks(&Fcb->FileLock);
#else
					OplockCount = (ULONG)FsRtlAreThereCurrentFileLocks(&Fcb->FileLock);
#endif
				}
			}
			else {
				// Shouldn't be something like UncleanCount counter and not FileCount
				// here?
				OplockCount = Fcb->FileCount;
			}
			}
		else if ((FsControlCode == FSCTL_OPLOCK_BREAK_ACKNOWLEDGE) ||
			(FsControlCode == FSCTL_OPBATCH_ACK_CLOSE_PENDING) ||
			(FsControlCode == FSCTL_OPLOCK_BREAK_NOTIFY) ||
			(FsControlCode == FSCTL_OPLOCK_BREAK_ACK_NO_2)
#if (NTDDI_VERSION >= NTDDI_WIN7)
			|| ((FsControlCode == FSCTL_REQUEST_OPLOCK) &&
				FlagOn(InputBuffer->Flags, REQUEST_OPLOCK_INPUT_FLAG_ACK))
#endif
			) {

			AcquiredFcb = ExAcquireResourceSharedLite(&(Fcb->Resource), TRUE);
#if (NTDDI_VERSION >= NTDDI_WIN7)
		}
		else if (FsControlCode == FSCTL_REQUEST_OPLOCK) {
			//
			//  The caller didn't provide either REQUEST_OPLOCK_INPUT_FLAG_REQUEST or
			//  REQUEST_OPLOCK_INPUT_FLAG_ACK on the input buffer.
			//
			DDbgPrint("    DokanOplockRequest STATUS_INVALID_PARAMETER\n");
			return STATUS_INVALID_PARAMETER;
		}
		else {
#else
		}
		else {
#endif
			DDbgPrint("    DokanOplockRequest STATUS_INVALID_PARAMETER\n");
			return STATUS_INVALID_PARAMETER;
		}

		//
		//  Fail batch, filter, and handle oplock requests if the file is marked
		//  for delete.
		//
		if (((FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK) ||
			(FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)
#if (NTDDI_VERSION >= NTDDI_WIN7)
			||
			((FsControlCode == FSCTL_REQUEST_OPLOCK) &&
				FlagOn(InputBuffer->RequestedOplockLevel, OPLOCK_LEVEL_CACHE_HANDLE))
#endif
			) &&
			FlagOn(Fcb->Flags, DOKAN_DELETE_ON_CLOSE)) {

			DDbgPrint("    DokanOplockRequest STATUS_DELETE_PENDING\n");
			return STATUS_DELETE_PENDING;
		}

		//
		//  Call the FsRtl routine to grant/acknowledge oplock.
		//
		Status = FsRtlOplockFsctrl(DokanGetFcbOplock(Fcb), Irp, OplockCount);

		//
		//  Once we call FsRtlOplockFsctrl, we no longer own the IRP and we should
		//  not complete it.
		//
		*pIrp = NULL;

		}
	__finally {

		//
		//  Release all of our resources
		//
		if (AcquiredVcb) {
			ExReleaseResourceLite(&(Fcb->Vcb->Resource));
		}

		if (AcquiredFcb) {
			ExReleaseResourceLite(&(Fcb->Resource));
		}

		DDbgPrint("    DokanOplockRequest return 0x%x\n", Status);
	}

	return Status;
	}
