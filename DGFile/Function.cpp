
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

VOID SetFileObjectForVCB(__in PFILE_OBJECT FileObject, __in PDokanVCB Vcb) {
	FileObject->SectionObjectPointer = &Vcb->SectionObjectPointers;
	FileObject->FsContext = &Vcb->VolumeFileHeader;
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
