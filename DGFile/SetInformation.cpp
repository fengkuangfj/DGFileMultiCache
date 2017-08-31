
#include "Stdafx.h"
#include "SetInformation.h"



VOID DokanCompleteSetInformation(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo) {
	PIRP irp;
	PIO_STACK_LOCATION irpSp;
	NTSTATUS status;
	ULONG info = 0;
	PDokanCCB ccb;
	PDokanFCB fcb;
	UNICODE_STRING oldFileName;

	FILE_INFORMATION_CLASS infoClass;
	irp = IrpEntry->Irp;
	status = EventInfo->Status;

	__try {

		DDbgPrint("==> DokanCompleteSetInformation\n");

		irpSp = IrpEntry->IrpSp;

		ccb = (PDokanCCB)IrpEntry->FileObject->FsContext2;
		ASSERT(ccb != NULL);

		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&ccb->Resource, TRUE);

		fcb = ccb->Fcb;
		ASSERT(fcb != NULL);

		ccb->UserContext = EventInfo->Context;

		info = EventInfo->BufferLength;

		infoClass = irpSp->Parameters.SetFile.FileInformationClass;

		RtlZeroMemory(&oldFileName, sizeof(UNICODE_STRING));

		if (NT_SUCCESS(status)) {

			if (infoClass == FileDispositionInformation) {

				if (EventInfo->Operation.Delete.DeleteOnClose) {

					if (!MmFlushImageSection(&fcb->SectionObjectPointers,
						MmFlushForDelete)) {
						DDbgPrint("  Cannot delete user mapped image\n");
						status = STATUS_CANNOT_DELETE;
					}
					else {
						ccb->Flags |= DOKAN_DELETE_ON_CLOSE;
						fcb->Flags |= DOKAN_DELETE_ON_CLOSE;
						DDbgPrint("   FileObject->DeletePending = TRUE\n");
						IrpEntry->FileObject->DeletePending = TRUE;
					}

				}
				else {
					ccb->Flags &= ~DOKAN_DELETE_ON_CLOSE;
					fcb->Flags &= ~DOKAN_DELETE_ON_CLOSE;
					DDbgPrint("   FileObject->DeletePending = FALSE\n");
					IrpEntry->FileObject->DeletePending = FALSE;
				}
			}

			// if rename is executed, reassign the file name
			if (infoClass == FileRenameInformation) {
				PVOID buffer = NULL;

				ExAcquireResourceExclusiveLite(&fcb->Resource, TRUE);

				// this is used to inform rename in the bellow switch case
				oldFileName.Buffer = fcb->FileName.Buffer;
				oldFileName.Length = (USHORT)fcb->FileName.Length;
				oldFileName.MaximumLength = (USHORT)fcb->FileName.Length;

				// copy new file name
				buffer = ExAllocatePool(EventInfo->BufferLength + sizeof(WCHAR));

				if (buffer == NULL) {
					status = STATUS_INSUFFICIENT_RESOURCES;
					ExReleaseResourceLite(&fcb->Resource);
					ExReleaseResourceLite(&ccb->Resource);
					KeLeaveCriticalRegion();
					__leave;
				}

				fcb->FileName.Buffer = (PWCH)buffer;

				ASSERT(fcb->FileName.Buffer != NULL);

				RtlZeroMemory(fcb->FileName.Buffer,
					EventInfo->BufferLength + sizeof(WCHAR));
				RtlCopyMemory(fcb->FileName.Buffer, EventInfo->Buffer,
					EventInfo->BufferLength);

				fcb->FileName.Length = (USHORT)EventInfo->BufferLength;
				fcb->FileName.MaximumLength = (USHORT)EventInfo->BufferLength;

				ExReleaseResourceLite(&fcb->Resource);
			}
		}

		ExReleaseResourceLite(&ccb->Resource);
		KeLeaveCriticalRegion();

		if (NT_SUCCESS(status)) {
			switch (irpSp->Parameters.SetFile.FileInformationClass) {
			case FileAllocationInformation:
				DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_SIZE,
					FILE_ACTION_MODIFIED);
				break;
			case FileBasicInformation:
				DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_ATTRIBUTES |
					FILE_NOTIFY_CHANGE_LAST_WRITE |
					FILE_NOTIFY_CHANGE_LAST_ACCESS |
					FILE_NOTIFY_CHANGE_CREATION,
					FILE_ACTION_MODIFIED);
				break;
			case FileDispositionInformation:
				if (IrpEntry->FileObject->DeletePending) {
					if (fcb->Flags & DOKAN_FILE_DIRECTORY) {
						DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_DIR_NAME,
							FILE_ACTION_REMOVED);
					}
					else {
						DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_FILE_NAME,
							FILE_ACTION_REMOVED);
					}
				}
				break;
			case FileEndOfFileInformation:
				DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_SIZE,
					FILE_ACTION_MODIFIED);
				break;
			case FileLinkInformation:
				// TODO: should check whether this is a directory
				// TODO: should notify new link name
				// DokanNotifyReportChange(vcb, ccb, FILE_NOTIFY_CHANGE_FILE_NAME,
				// FILE_ACTION_ADDED);
				break;
			case FilePositionInformation:
				// this is never used
				break;
			case FileRenameInformation: {
				DokanNotifyReportChange0(fcb, &oldFileName,
					FILE_NOTIFY_CHANGE_FILE_NAME,
					FILE_ACTION_RENAMED_OLD_NAME);

				// free old file name
				ExFreePool(oldFileName.Buffer);

				DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_FILE_NAME,
					FILE_ACTION_RENAMED_NEW_NAME);
			} break;
			case FileValidDataLengthInformation:
				DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_SIZE,
					FILE_ACTION_MODIFIED);
				break;
			default:
				DDbgPrint("  unknown type:%d\n",
					irpSp->Parameters.SetFile.FileInformationClass);
				break;
			}
		}

	}
	__finally {

		DokanCompleteIrpRequest(irp, status, info);

		DDbgPrint("<== DokanCompleteSetInformation\n");
	}
}
