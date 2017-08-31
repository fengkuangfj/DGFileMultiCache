
#include "Stdafx.h"
#include "Function.h"

NTSTATUS DokanGetParentDir(__in const WCHAR *fileName, __out WCHAR **parentDir,
	__out ULONG *parentDirLength) {
	// first check if there is a parent

	LONG len = (LONG)wcslen(fileName);

	LONG i;

	BOOLEAN trailingSlash;

	*parentDir = NULL;
	*parentDirLength = 0;

	if (len < 1) {
		return STATUS_INVALID_PARAMETER;
	}

	if (!wcscmp(fileName, L"\\"))
		return STATUS_ACCESS_DENIED;

	trailingSlash = fileName[len - 1] == '\\';

	*parentDir = (WCHAR *)ExAllocatePool((len + 1) * sizeof(WCHAR));

	if (!*parentDir)
		return STATUS_INSUFFICIENT_RESOURCES;

	wcscpy(*parentDir, fileName);

	for (i = len - 1; i >= 0; i--) {
		if ((*parentDir)[i] == '\\') {
			if (i == len - 1 && trailingSlash) {
				continue;
			}
			(*parentDir)[i] = 0;
			break;
		}
	}

	if (i <= 0) {
		i = 1;
		(*parentDir)[0] = '\\';
		(*parentDir)[1] = 0;
	}

	*parentDirLength = i * sizeof(WCHAR);
	if (trailingSlash && i > 1) {
		(*parentDir)[i] = '\\';
		(*parentDir)[i + 1] = 0;
		*parentDirLength += sizeof(WCHAR);
	}

	return STATUS_SUCCESS;
}

LONG DokanUnicodeStringChar(__in PUNICODE_STRING UnicodeString,
	__in WCHAR Char) {
	ULONG i = 0;
	for (; i < UnicodeString->Length / sizeof(WCHAR); ++i) {
		if (UnicodeString->Buffer[i] == Char) {
			return i;
		}
	}
	return -1;
}

NTSTATUS
DokanCheckShareAccess(__in PFILE_OBJECT FileObject, __in PDokanFCB FcbOrDcb,
	__in ACCESS_MASK DesiredAccess, __in ULONG ShareAccess)

	/*++
	Routine Description:
	This routine checks conditions that may result in a sharing violation.
	Arguments:
	FileObject - Pointer to the file object of the current open request.
	FcbOrDcb - Supplies a pointer to the Fcb/Dcb.
	DesiredAccess - Desired access of current open request.
	ShareAccess - Shared access requested by current open request.
	Return Value:
	If the accessor has access to the file, STATUS_SUCCESS is returned.
	Otherwise, STATUS_SHARING_VIOLATION is returned.

	--*/

{
	NTSTATUS status;
	PAGED_CODE();

#if (NTDDI_VERSION >= NTDDI_VISTA)
	//
	//  Do an extra test for writeable user sections if the user did not allow
	//  write sharing - this is neccessary since a section may exist with no
	//  handles
	//  open to the file its based against.
	//
	if ((FcbOrDcb->Identifier.Type == FCB) &&
		!FlagOn(ShareAccess, FILE_SHARE_WRITE) &&
		FlagOn(DesiredAccess, FILE_EXECUTE | FILE_READ_DATA | FILE_WRITE_DATA |
			FILE_APPEND_DATA | DELETE | MAXIMUM_ALLOWED) &&
		MmDoesFileHaveUserWritableReferences(&FcbOrDcb->SectionObjectPointers)) {

		DDbgPrint("  DokanCheckShareAccess FCB has no write shared access\n");
		return STATUS_SHARING_VIOLATION;
	}
#endif

	//
	//  Check if the Fcb has the proper share access.
	//
	//  Pass FALSE for update.  We will update it later.
	ExAcquireResourceExclusiveLite(&FcbOrDcb->Resource, TRUE);
	status = IoCheckShareAccess(DesiredAccess, ShareAccess, FileObject,
		&FcbOrDcb->ShareAccess, FALSE);
	ExReleaseResourceLite(&FcbOrDcb->Resource);

	return status;
}

NTSTATUS
DokanUserFsRequest(__in PDEVICE_OBJECT DeviceObject, __in PIRP *pIrp) {
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	PIO_STACK_LOCATION irpSp;

	UNREFERENCED_PARAMETER(DeviceObject);

	irpSp = IoGetCurrentIrpStackLocation(*pIrp);

	switch (irpSp->Parameters.FileSystemControl.FsControlCode) {

	case FSCTL_REQUEST_OPLOCK_LEVEL_1:
		DDbgPrint("    FSCTL_REQUEST_OPLOCK_LEVEL_1\n");
		status = DokanOplockRequest(pIrp);
		break;

	case FSCTL_REQUEST_OPLOCK_LEVEL_2:
		DDbgPrint("    FSCTL_REQUEST_OPLOCK_LEVEL_2\n");
		status = DokanOplockRequest(pIrp);
		break;

	case FSCTL_REQUEST_BATCH_OPLOCK:
		DDbgPrint("    FSCTL_REQUEST_BATCH_OPLOCK\n");
		status = DokanOplockRequest(pIrp);
		break;

	case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
		DDbgPrint("    FSCTL_OPLOCK_BREAK_ACKNOWLEDGE\n");
		status = DokanOplockRequest(pIrp);
		break;

	case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
		DDbgPrint("    FSCTL_OPBATCH_ACK_CLOSE_PENDING\n");
		status = DokanOplockRequest(pIrp);
		break;

	case FSCTL_OPLOCK_BREAK_NOTIFY:
		DDbgPrint("    FSCTL_OPLOCK_BREAK_NOTIFY\n");
		status = DokanOplockRequest(pIrp);
		break;

	case FSCTL_OPLOCK_BREAK_ACK_NO_2:
		DDbgPrint("    FSCTL_OPLOCK_BREAK_ACK_NO_2\n");
		status = DokanOplockRequest(pIrp);
		break;

	case FSCTL_REQUEST_FILTER_OPLOCK:
		DDbgPrint("    FSCTL_REQUEST_FILTER_OPLOCK\n");
		status = DokanOplockRequest(pIrp);
		break;

#if (NTDDI_VERSION >= NTDDI_WIN7)
	case FSCTL_REQUEST_OPLOCK:
		DDbgPrint("    FSCTL_REQUEST_OPLOCK\n");
		status = DokanOplockRequest(pIrp);
		break;
#endif

	case FSCTL_LOCK_VOLUME:
		DDbgPrint("    FSCTL_LOCK_VOLUME\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_UNLOCK_VOLUME:
		DDbgPrint("    FSCTL_UNLOCK_VOLUME\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_DISMOUNT_VOLUME:
		DDbgPrint("    FSCTL_DISMOUNT_VOLUME\n");
		break;

	case FSCTL_IS_VOLUME_MOUNTED:
		DDbgPrint("    FSCTL_IS_VOLUME_MOUNTED\n");
		status = STATUS_SUCCESS;
		break;

	case FSCTL_IS_PATHNAME_VALID:
		DDbgPrint("    FSCTL_IS_PATHNAME_VALID\n");
		break;

	case FSCTL_MARK_VOLUME_DIRTY:
		DDbgPrint("    FSCTL_MARK_VOLUME_DIRTY\n");
		break;

	case FSCTL_QUERY_RETRIEVAL_POINTERS:
		DDbgPrint("    FSCTL_QUERY_RETRIEVAL_POINTERS\n");
		break;

	case FSCTL_GET_COMPRESSION:
		DDbgPrint("    FSCTL_GET_COMPRESSION\n");
		break;

	case FSCTL_SET_COMPRESSION:
		DDbgPrint("    FSCTL_SET_COMPRESSION\n");
		break;

	case FSCTL_MARK_AS_SYSTEM_HIVE:
		DDbgPrint("    FSCTL_MARK_AS_SYSTEM_HIVE\n");
		break;

	case FSCTL_INVALIDATE_VOLUMES:
		DDbgPrint("    FSCTL_INVALIDATE_VOLUMES\n");
		break;

	case FSCTL_QUERY_FAT_BPB:
		DDbgPrint("    FSCTL_QUERY_FAT_BPB\n");
		break;

	case FSCTL_FILESYSTEM_GET_STATISTICS:
		DDbgPrint("    FSCTL_FILESYSTEM_GET_STATISTICS\n");
		break;

	case FSCTL_GET_NTFS_VOLUME_DATA:
		DDbgPrint("    FSCTL_GET_NTFS_VOLUME_DATA\n");
		break;

	case FSCTL_GET_NTFS_FILE_RECORD:
		DDbgPrint("    FSCTL_GET_NTFS_FILE_RECORD\n");
		break;

	case FSCTL_GET_VOLUME_BITMAP:
		DDbgPrint("    FSCTL_GET_VOLUME_BITMAP\n");
		break;

	case FSCTL_GET_RETRIEVAL_POINTERS:
		DDbgPrint("    FSCTL_GET_RETRIEVAL_POINTERS\n");
		break;

	case FSCTL_MOVE_FILE:
		DDbgPrint("    FSCTL_MOVE_FILE\n");
		break;

	case FSCTL_IS_VOLUME_DIRTY:
		DDbgPrint("    FSCTL_IS_VOLUME_DIRTY\n");
		break;

	case FSCTL_ALLOW_EXTENDED_DASD_IO:
		DDbgPrint("    FSCTL_ALLOW_EXTENDED_DASD_IO\n");
		break;

	case FSCTL_FIND_FILES_BY_SID:
		DDbgPrint("    FSCTL_FIND_FILES_BY_SID\n");
		break;

	case FSCTL_SET_OBJECT_ID:
		DDbgPrint("    FSCTL_SET_OBJECT_ID\n");
		break;

	case FSCTL_GET_OBJECT_ID:
		DDbgPrint("    FSCTL_GET_OBJECT_ID\n");
		break;

	case FSCTL_DELETE_OBJECT_ID:
		DDbgPrint("    FSCTL_DELETE_OBJECT_ID\n");
		break;

	case FSCTL_SET_REPARSE_POINT:
		DDbgPrint("    FSCTL_SET_REPARSE_POINT\n");
		break;

	case FSCTL_GET_REPARSE_POINT:
		DDbgPrint("    FSCTL_GET_REPARSE_POINT\n");
		status = STATUS_NOT_A_REPARSE_POINT;
		break;

	case FSCTL_DELETE_REPARSE_POINT:
		DDbgPrint("    FSCTL_DELETE_REPARSE_POINT\n");
		break;

	case FSCTL_ENUM_USN_DATA:
		DDbgPrint("    FSCTL_ENUM_USN_DATA\n");
		break;

	case FSCTL_SECURITY_ID_CHECK:
		DDbgPrint("    FSCTL_SECURITY_ID_CHECK\n");
		break;

	case FSCTL_READ_USN_JOURNAL:
		DDbgPrint("    FSCTL_READ_USN_JOURNAL\n");
		break;

	case FSCTL_SET_OBJECT_ID_EXTENDED:
		DDbgPrint("    FSCTL_SET_OBJECT_ID_EXTENDED\n");
		break;

	case FSCTL_CREATE_OR_GET_OBJECT_ID:
		DDbgPrint("    FSCTL_CREATE_OR_GET_OBJECT_ID\n");
		break;

	case FSCTL_SET_SPARSE:
		DDbgPrint("    FSCTL_SET_SPARSE\n");
		break;

	case FSCTL_SET_ZERO_DATA:
		DDbgPrint("    FSCTL_SET_ZERO_DATA\n");
		break;

	case FSCTL_QUERY_ALLOCATED_RANGES:
		DDbgPrint("    FSCTL_QUERY_ALLOCATED_RANGES\n");
		break;

	case FSCTL_SET_ENCRYPTION:
		DDbgPrint("    FSCTL_SET_ENCRYPTION\n");
		break;

	case FSCTL_ENCRYPTION_FSCTL_IO:
		DDbgPrint("    FSCTL_ENCRYPTION_FSCTL_IO\n");
		break;

	case FSCTL_WRITE_RAW_ENCRYPTED:
		DDbgPrint("    FSCTL_WRITE_RAW_ENCRYPTED\n");
		break;

	case FSCTL_READ_RAW_ENCRYPTED:
		DDbgPrint("    FSCTL_READ_RAW_ENCRYPTED\n");
		break;

	case FSCTL_CREATE_USN_JOURNAL:
		DDbgPrint("    FSCTL_CREATE_USN_JOURNAL\n");
		break;

	case FSCTL_READ_FILE_USN_DATA:
		DDbgPrint("    FSCTL_READ_FILE_USN_DATA\n");
		break;

	case FSCTL_WRITE_USN_CLOSE_RECORD:
		DDbgPrint("    FSCTL_WRITE_USN_CLOSE_RECORD\n");
		break;

	case FSCTL_EXTEND_VOLUME:
		DDbgPrint("    FSCTL_EXTEND_VOLUME\n");
		break;

	case FSCTL_QUERY_USN_JOURNAL:
		DDbgPrint("    FSCTL_QUERY_USN_JOURNAL\n");
		break;

	case FSCTL_DELETE_USN_JOURNAL:
		DDbgPrint("    FSCTL_DELETE_USN_JOURNAL\n");
		break;

	case FSCTL_MARK_HANDLE:
		DDbgPrint("    FSCTL_MARK_HANDLE\n");
		break;

	case FSCTL_SIS_COPYFILE:
		DDbgPrint("    FSCTL_SIS_COPYFILE\n");
		break;

	case FSCTL_SIS_LINK_FILES:
		DDbgPrint("    FSCTL_SIS_LINK_FILES\n");
		break;

	case FSCTL_RECALL_FILE:
		DDbgPrint("    FSCTL_RECALL_FILE\n");
		break;

	case FSCTL_SET_ZERO_ON_DEALLOCATION:
		DDbgPrint("    FSCTL_SET_ZERO_ON_DEALLOCATION\n");
		break;

	case FSCTL_CSC_INTERNAL:
		DDbgPrint("    FSCTL_CSC_INTERNAL\n");
		break;

	case FSCTL_QUERY_ON_DISK_VOLUME_INFO:
		DDbgPrint("    FSCTL_QUERY_ON_DISK_VOLUME_INFO\n");
		break;

	default:
		DDbgPrint("    Unknown FSCTL %d\n",
			(irpSp->Parameters.FileSystemControl.FsControlCode >> 2) & 0xFFF);
		status = STATUS_INVALID_DEVICE_REQUEST;
	}

	return status;
}

PUNICODE_STRING
DokanAllocateUnicodeString(__in PCWSTR String) {
	PUNICODE_STRING unicode;
	PWSTR buffer;
	ULONG length;
	unicode = (PUNICODE_STRING)ExAllocatePool(sizeof(UNICODE_STRING));
	if (unicode == NULL) {
		return NULL;
	}

	length = (ULONG)(wcslen(String) + 1) * sizeof(WCHAR);
	buffer = (PWSTR)ExAllocatePool(length);
	if (buffer == NULL) {
		ExFreePool(unicode);
		return NULL;
	}
	RtlCopyMemory(buffer, String, length);
	RtlInitUnicodeString(unicode, buffer);
	return unicode;
}

VOID FreeUnicodeString(PUNICODE_STRING UnicodeString) {
	if (UnicodeString != NULL) {
		ExFreePool(UnicodeString->Buffer);
		ExFreePool(UnicodeString);
	}
}

ULONG
PointerAlignSize(ULONG sizeInBytes) {
	// power of 2 cheat to avoid using %
	ULONG remainder = sizeInBytes & (sizeof(void *) - 1);

	if (remainder > 0) {
		return sizeInBytes + (sizeof(void *) - remainder);
	}

	return sizeInBytes;
}

VOID DokanPrintNTStatus(NTSTATUS Status) {
	DDbgPrint("  status = 0x%x\n", Status);

	PrintStatus(Status, STATUS_SUCCESS);
	PrintStatus(Status, STATUS_PENDING);
	PrintStatus(Status, STATUS_NO_MORE_FILES);
	PrintStatus(Status, STATUS_END_OF_FILE);
	PrintStatus(Status, STATUS_NO_SUCH_FILE);
	PrintStatus(Status, STATUS_NOT_IMPLEMENTED);
	PrintStatus(Status, STATUS_BUFFER_OVERFLOW);
	PrintStatus(Status, STATUS_FILE_IS_A_DIRECTORY);
	PrintStatus(Status, STATUS_SHARING_VIOLATION);
	PrintStatus(Status, STATUS_OBJECT_NAME_INVALID);
	PrintStatus(Status, STATUS_OBJECT_NAME_NOT_FOUND);
	PrintStatus(Status, STATUS_OBJECT_NAME_COLLISION);
	PrintStatus(Status, STATUS_OBJECT_PATH_INVALID);
	PrintStatus(Status, STATUS_OBJECT_PATH_NOT_FOUND);
	PrintStatus(Status, STATUS_OBJECT_PATH_SYNTAX_BAD);
	PrintStatus(Status, STATUS_ACCESS_DENIED);
	PrintStatus(Status, STATUS_ACCESS_VIOLATION);
	PrintStatus(Status, STATUS_INVALID_PARAMETER);
	PrintStatus(Status, STATUS_INVALID_USER_BUFFER);
	PrintStatus(Status, STATUS_INVALID_HANDLE);
	PrintStatus(Status, STATUS_INSUFFICIENT_RESOURCES);
	PrintStatus(Status, STATUS_DEVICE_DOES_NOT_EXIST);
	PrintStatus(Status, STATUS_INVALID_DEVICE_REQUEST);
	PrintStatus(Status, STATUS_VOLUME_DISMOUNTED);
}

VOID PrintIdType(__in VOID *Id) {
	if (Id == NULL) {
		DDbgPrint("    IdType = NULL\n");
		return;
	}
	switch (GetIdentifierType(Id)) {
	case DGL:
		DDbgPrint("    IdType = DGL\n");
		break;
	case DCB:
		DDbgPrint("   IdType = DCB\n");
		break;
	case VCB:
		DDbgPrint("   IdType = VCB\n");
		break;
	case FCB:
		DDbgPrint("   IdType = FCB\n");
		break;
	case CCB:
		DDbgPrint("   IdType = CCB\n");
		break;
	default:
		DDbgPrint("   IdType = Unknown\n");
		break;
	}
}

BOOLEAN
DokanNoOpAcquire(__in PVOID Fcb, __in BOOLEAN Wait) {
	UNREFERENCED_PARAMETER(Fcb);
	UNREFERENCED_PARAMETER(Wait);

	DDbgPrint("==> DokanNoOpAcquire\n");

	ASSERT(IoGetTopLevelIrp() == NULL);

	IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

	DDbgPrint("<== DokanNoOpAcquire\n");

	return TRUE;
}

VOID DokanNoOpRelease(__in PVOID Fcb) {
	DDbgPrint("==> DokanNoOpRelease\n");
	ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

	IoSetTopLevelIrp(NULL);

	UNREFERENCED_PARAMETER(Fcb);

	DDbgPrint("<== DokanNoOpRelease\n");
	return;
}

VOID DokanNotifyReportChange0(__in PDokanFCB Fcb, __in PUNICODE_STRING FileName,
	__in ULONG FilterMatch, __in ULONG Action) {
	USHORT nameOffset;

	DDbgPrint("==> DokanNotifyReportChange %wZ\n", FileName);

	ASSERT(Fcb != NULL);
	ASSERT(FileName != NULL);

	// search the last "\"
	nameOffset = (USHORT)(FileName->Length / sizeof(WCHAR) - 1);
	for (; FileName->Buffer[nameOffset] != L'\\'; --nameOffset)
		;
	nameOffset++; // the next is the begining of filename

	nameOffset *= sizeof(WCHAR); // Offset is in bytes

	FsRtlNotifyFullReportChange(Fcb->Vcb->NotifySync, &Fcb->Vcb->DirNotifyList,
		(PSTRING)FileName, nameOffset,
		NULL, // StreamName
		NULL, // NormalizedParentName
		FilterMatch, Action,
		NULL); // TargetContext

	DDbgPrint("<== DokanNotifyReportChange\n");
}

VOID DokanNotifyReportChange(__in PDokanFCB Fcb, __in ULONG FilterMatch,
	__in ULONG Action) {
	ASSERT(Fcb != NULL);
	DokanNotifyReportChange0(Fcb, &Fcb->FileName, FilterMatch, Action);
}

BOOLEAN
DokanLookasideCreate(LOOKASIDE_LIST_EX *pCache, size_t cbElement) {
	NTSTATUS Status = ExInitializeLookasideListEx(
		pCache, NULL, NULL, NonPagedPool, 0, cbElement, TAG, 0);

	if (!NT_SUCCESS(Status)) {
		DDbgPrint("ExInitializeLookasideListEx failed, Status (0x%x)", Status);
		return FALSE;
	}

	return TRUE;
}

NTSTATUS
DokanGetAccessToken(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	KIRQL oldIrql = 0;
	PLIST_ENTRY thisEntry, nextEntry, listHead;
	PIRP_ENTRY irpEntry;
	PDokanVCB vcb;
	PEVENT_INFORMATION eventInfo;
	PACCESS_TOKEN accessToken;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	HANDLE handle;
	PIO_STACK_LOCATION irpSp = NULL;
	BOOLEAN hasLock = FALSE;
	ULONG outBufferLen;
	ULONG inBufferLen;
	PACCESS_STATE accessState = NULL;

	DDbgPrint("==> DokanGetAccessToken\n");
	vcb = (PDokanVCB)DeviceObject->DeviceExtension;

	__try {
		eventInfo = (PEVENT_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
		ASSERT(eventInfo != NULL);

		if (Irp->RequestorMode != UserMode) {
			DDbgPrint("  needs to be called from user-mode\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		if (GetIdentifierType(vcb) != VCB) {
			DDbgPrint("  GetIdentifierType != VCB\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		outBufferLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
		inBufferLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
		if (outBufferLen != sizeof(EVENT_INFORMATION) ||
			inBufferLen != sizeof(EVENT_INFORMATION)) {
			DDbgPrint("  wrong input or output buffer length\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
		KeAcquireSpinLock(&vcb->Dcb->PendingIrp.ListLock, &oldIrql);
		hasLock = TRUE;

		// search corresponding IRP through pending IRP list
		listHead = &vcb->Dcb->PendingIrp.ListHead;

		for (thisEntry = listHead->Flink; thisEntry != listHead;
			thisEntry = nextEntry) {

			nextEntry = thisEntry->Flink;

			irpEntry = CONTAINING_RECORD(thisEntry, IRP_ENTRY, ListEntry);

			if (irpEntry->SerialNumber != eventInfo->SerialNumber) {
				continue;
			}

			// this irp must be IRP_MJ_CREATE
			if (irpEntry->IrpSp->Parameters.Create.SecurityContext) {
				accessState =
					irpEntry->IrpSp->Parameters.Create.SecurityContext->AccessState;
			}
			break;
		}
		KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);
		hasLock = FALSE;

		if (accessState == NULL) {
			DDbgPrint("  can't find pending Irp: %d\n", eventInfo->SerialNumber);
			__leave;
		}

		accessToken =
			SeQuerySubjectContextToken(&accessState->SubjectSecurityContext);
		if (accessToken == NULL) {
			DDbgPrint("  accessToken == NULL\n");
			__leave;
		}
		// NOTE: Accessing *SeTokenObjectType while acquring sping lock causes
		// BSOD on Windows XP.
		status = ObOpenObjectByPointer(accessToken, 0, NULL, GENERIC_ALL,
			*SeTokenObjectType, KernelMode, &handle);
		if (!NT_SUCCESS(status)) {
			DDbgPrint("  ObOpenObjectByPointer failed: 0x%x\n", status);
			__leave;
		}

		eventInfo->Operation.AccessToken.Handle = handle;
		Irp->IoStatus.Information = sizeof(EVENT_INFORMATION);
		status = STATUS_SUCCESS;

	}
	__finally {
		if (hasLock) {
			KeReleaseSpinLock(&vcb->Dcb->PendingIrp.ListLock, oldIrql);
		}
	}
	DDbgPrint("<== DokanGetAccessToken\n");
	return status;
}
