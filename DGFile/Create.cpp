
#include "Stdafx.h"
#include "Create.h"

NTSTATUS
CDGFile::Create(
	__in PDEVICE_OBJECT pDeviceObject,
	__in PIRP			pIrp
)
{
	NTSTATUS			ntStatus = STATUS_INVALID_PARAMETER;

	PIO_STACK_LOCATION	pIoStackLocation = NULL;
	PFILE_OBJECT		pFileObject = NULL;
	PFILE_OBJECT		pRelatedFileObject = NULL;
	LPVCB				lpVcb = NULL;
	FSD_IDENTIFIER		FsdIdentifier = { (FSD_IDENTIFIER_TYPE)0, 0 };
	LPDCB				lpDcb = NULL;
	LPCCB				lpCcbRelated = NULL;
	DWORD				CreateDisposition = 0;
	DWORD				CreateOptions = 0;
	ULONG				info = 0;

	CKrnlStr			strFileName;
	CKrnlStr			strFileNameRelated;

	
	PEVENT_CONTEXT eventContext = NULL;
	
	ULONG fileNameLength = 0;
	ULONG eventLength;
	PDokanFCB fcb = NULL;
	PDokanCCB ccb = NULL;
	PWCHAR fileName = NULL;
	PWCHAR parentDir = NULL; // for SL_OPEN_TARGET_DIRECTORY
	ULONG parentDirLength = 0;
	BOOLEAN needBackSlashAfterRelatedFile = FALSE;
	ULONG securityDescriptorSize = 0;
	ULONG alignedEventContextSize = 0;
	ULONG alignedObjectNameSize = PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE));
	ULONG alignedObjectTypeNameSize = PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE));
	PDOKAN_UNICODE_STRING_INTERMEDIATE intermediateUnicodeStr = NULL;
	PSECURITY_DESCRIPTOR newFileSecurityDescriptor = NULL;
	BOOLEAN OpenRequiringOplock = FALSE;
	BOOLEAN UnwindShareAccess = FALSE;
	BOOLEAN BackoutOplock = FALSE;
	BOOLEAN EventContextConsumed = FALSE;
	

	PAGED_CODE();

	__try
	{
		pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);
		if (!pIoStackLocation)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IoGetCurrentIrpStackLocation failed.");
			__leave;
		}

		pFileObject = pIoStackLocation->FileObject;
		if (!pFileObject)
		{
			ntStatus = STATUS_INVALID_PARAMETER;
			__leave;
		}

		pRelatedFileObject = pFileObject->RelatedFileObject;

		CreateDisposition = (pIoStackLocation->Parameters.Create.Options >> 24) & 0x000000FF;
		CreateOptions = pIoStackLocation->Parameters.Create.Options & 0x00FFFFFF;

		if (!pDeviceObject->DeviceExtension)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}

		FsdIdentifier = ((LPVCB)(pDeviceObject->DeviceExtension))->Identifier;

		switch (GetFsdIdentifierType(&FsdIdentifier))
		{
		case FSD_IDENTIFIER_TYPE_DCB:
		{
			lpDcb = (LPDCB)pDeviceObject->DeviceExtension;
			if (!lpDcb->Mounted)
			{
				ntStatus = STATUS_VOLUME_DISMOUNTED;
				__leave;
			}

			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		case FSD_IDENTIFIER_TYPE_VCB:
		{
			lpVcb = (LPVCB)pDeviceObject->DeviceExtension;
			lpDcb = lpVcb->Dcb;
			break;
		}
		default:
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}
		}

		if (FILE_DEVICE_NETWORK_FILE_SYSTEM != lpDcb->VolumeDeviceType)
		{
			if (pRelatedFileObject)
				pFileObject->Vpb = pRelatedFileObject->Vpb;
			else
				pFileObject->Vpb = lpDcb->DeviceObject->Vpb;
		}

		if (!lpVcb->HasEventWait)
		{
			ntStatus = STATUS_SUCCESS;
			__leave;
		}

		if (pFileObject->FileName.Buffer &&
			pFileObject->FileName.Length)
			strFileName.Set(&pFileObject->FileName);

		if (strFileName.GetString() == strFileName.Search(L"\\\\", TRUE))
			strFileName.Set(strFileName.GetString() + 1, strFileName.GetLenB() - 1);

		if (pRelatedFileObject)
		{
			if (pRelatedFileObject->FsContext2)
			{
				lpCcbRelated = (LPCCB)pRelatedFileObject->FsContext2;
				if (lpCcbRelated &&
					lpCcbRelated->Fcb &&
					lpCcbRelated->Fcb->FileName.Buffer &&
					lpCcbRelated->Fcb->FileName.Length)
					strFileNameRelated.Set(&lpCcbRelated->Fcb->FileName);
			}
		}

		if (!strFileNameRelated.GetLenCh() && !strFileName.GetLenCh())
		{
			if (FlagOn(CreateOptions, FILE_DIRECTORY_FILE))
				ntStatus = STATUS_NOT_A_DIRECTORY;
			else
			{
				SetFileObjectForVCB(pFileObject, lpVcb);
				info = FILE_OPENED;
				ntStatus = STATUS_SUCCESS;
			}

			__leave;
		}

		if (L'\\' == *(strFileName.GetString() + strFileName.GetLenCh() - 1))
			strFileName.Shorten(strFileName.GetLenCh() - 1);

		fileNameLength = pFileObject->FileName.Length;
		if (relatedFileName != NULL)
		{
			fileNameLength += relatedFileName->Length;

			if (pFileObject->FileName.Length > 0 &&
				pFileObject->FileName.Buffer[0] == '\\')
			{
				DDbgPrint("  when RelatedFileObject is specified, the file name should "
					"be relative path\n");
				ntStatus = STATUS_OBJECT_NAME_INVALID;
				__leave;
			}
			if (relatedFileName->Length > 0 &&
				relatedFileName->Buffer[relatedFileName->Length / sizeof(WCHAR) -
				1] != '\\') {
				needBackSlashAfterRelatedFile = TRUE;
				fileNameLength += sizeof(WCHAR);
			}
		}

		// don't open file like stream
		if (!lpDcb->UseAltStream &&
			DokanUnicodeStringChar(&pFileObject->FileName, L':') != -1) {
			DDbgPrint("    alternate stream\n");
			ntStatus = STATUS_INVALID_PARAMETER;
			info = 0;
			__leave;
		}

		// this memory is freed by DokanGetFCB if needed
		// "+ sizeof(WCHAR)" is for the last NULL character
		fileName = ExAllocatePool(fileNameLength + sizeof(WCHAR));
		if (fileName == NULL) {
			DDbgPrint("    Can't allocatePool for fileName\n");
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		RtlZeroMemory(fileName, fileNameLength + sizeof(WCHAR));

		if (relatedFileName != NULL) {
			DDbgPrint("  RelatedFileName:%wZ\n", relatedFileName);

			// copy the file name of related file object
			RtlCopyMemory(fileName, relatedFileName->Buffer, relatedFileName->Length);

			if (needBackSlashAfterRelatedFile) {
				((PWCHAR)fileName)[relatedFileName->Length / sizeof(WCHAR)] = '\\';
			}
			// copy the file name of fileObject
			RtlCopyMemory((PCHAR)fileName + relatedFileName->Length +
				(needBackSlashAfterRelatedFile ? sizeof(WCHAR) : 0),
				pFileObject->FileName.Buffer, pFileObject->FileName.Length);

		}
		else {
			// if related file object is not specifed, copy the file name of file
			// object
			RtlCopyMemory(fileName, pFileObject->FileName.Buffer,
				pFileObject->FileName.Length);
		}

		// Fail if device is read-only and request involves a write operation

		if (IS_DEVICE_READ_ONLY(DeviceObject)) {

			if ((CreateDisposition == FILE_SUPERSEDE) || (CreateDisposition == FILE_CREATE) ||
				(CreateDisposition == FILE_OVERWRITE) ||
				(CreateDisposition == FILE_OVERWRITE_IF) ||
				(pIoStackLocation->Parameters.Create.Options & FILE_DELETE_ON_CLOSE)) {

				DDbgPrint("    Media is write protected\n");
				ntStatus = STATUS_MEDIA_WRITE_PROTECTED;
				ExFreePool(fileName);
				__leave;
			}
		}

		if (pIoStackLocation->Flags & SL_OPEN_TARGET_DIRECTORY) {
			ntStatus = DokanGetParentDir(fileName, &parentDir, &parentDirLength);
			if (ntStatus != STATUS_SUCCESS) {
				ExFreePool(fileName);
				__leave;
			}
			fcb = DokanGetFCB(lpVcb, parentDir, parentDirLength);
		}
		else {
			fcb = DokanGetFCB(lpVcb, fileName, fileNameLength);
		}
		if (fcb == NULL) {
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		if (fcb->FileCount > 1 && CreateDisposition == FILE_CREATE) {
			ntStatus = STATUS_OBJECT_NAME_COLLISION;
			__leave;
		}

		if (pIoStackLocation->Flags & SL_OPEN_PAGING_FILE) {
			fcb->AdvancedFCBHeader.Flags2 |= FSRTL_FLAG2_IS_PAGING_FILE;
			fcb->AdvancedFCBHeader.Flags2 &= ~FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS;
		}

		ccb = DokanAllocateCCB(lpDcb, fcb);
		if (ccb == NULL) {
			DDbgPrint("    Was not able to allocate CCB\n");
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		// remember FILE_DELETE_ON_CLOSE so than the file can be deleted in close
		// for windows 8
		if (pIoStackLocation->Parameters.Create.Options & FILE_DELETE_ON_CLOSE) {
			fcb->Flags |= DOKAN_DELETE_ON_CLOSE;
			ccb->Flags |= DOKAN_DELETE_ON_CLOSE;
			DDbgPrint(
				"  FILE_DELETE_ON_CLOSE is set so remember for delete in cleanup\n");
		}

		pFileObject->FsContext = &fcb->AdvancedFCBHeader;
		pFileObject->FsContext2 = ccb;
		pFileObject->PrivateCacheMap = NULL;
		pFileObject->SectionObjectPointer = &fcb->SectionObjectPointers;
		// fileObject->Flags |= FILE_NO_INTERMEDIATE_BUFFERING;

		alignedEventContextSize = PointerAlignSize(sizeof(EVENT_CONTEXT));

		if (pIoStackLocation->Parameters.Create.SecurityContext->AccessState) {

			if (pIoStackLocation->Parameters.Create.SecurityContext->AccessState
				->SecurityDescriptor) {
				// (CreateOptions & FILE_DIRECTORY_FILE) == FILE_DIRECTORY_FILE
				if (SeAssignSecurity(
					NULL, // we don't keep track of parents, this will have to be
						  // handled in user mode
					pIoStackLocation->Parameters.Create.SecurityContext->AccessState
					->SecurityDescriptor,
					&newFileSecurityDescriptor,
					(pIoStackLocation->Parameters.Create.Options & FILE_DIRECTORY_FILE) ||
					(pIoStackLocation->Flags & SL_OPEN_TARGET_DIRECTORY),
					&pIoStackLocation->Parameters.Create.SecurityContext->AccessState
					->SubjectSecurityContext,
					IoGetFileObjectGenericMapping(), PagedPool) == STATUS_SUCCESS) {

					securityDescriptorSize = PointerAlignSize(
						RtlLengthSecurityDescriptor(newFileSecurityDescriptor));
				}
				else {
					newFileSecurityDescriptor = NULL;
				}
			}

			if (pIoStackLocation->Parameters.Create.SecurityContext->AccessState->ObjectName
				.Length > 0) {
				// add 1 WCHAR for NULL
				alignedObjectNameSize =
					PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE) +
						pIoStackLocation->Parameters.Create.SecurityContext
						->AccessState->ObjectName.Length +
						sizeof(WCHAR));
			}
			// else alignedObjectNameSize =
			// PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE)) SEE
			// DECLARATION

			if (pIoStackLocation->Parameters.Create.SecurityContext->AccessState->ObjectTypeName
				.Length > 0) {
				// add 1 WCHAR for NULL
				alignedObjectTypeNameSize =
					PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE) +
						pIoStackLocation->Parameters.Create.SecurityContext
						->AccessState->ObjectTypeName.Length +
						sizeof(WCHAR));
			}
			// else alignedObjectTypeNameSize =
			// PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE)) SEE
			// DECLARATION
		}

		eventLength = alignedEventContextSize + securityDescriptorSize;
		eventLength += alignedObjectNameSize;
		eventLength += alignedObjectTypeNameSize;
		eventLength += (parentDir ? fileNameLength : fcb->FileName.Length) +
			sizeof(WCHAR); // add WCHAR for NULL

		eventContext = AllocateEventContext(lpVcb->Dcb, Irp, eventLength, ccb);

		if (eventContext == NULL) {
			DDbgPrint("    Was not able to allocate eventContext\n");
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		RtlZeroMemory((char *)eventContext + alignedEventContextSize,
			eventLength - alignedEventContextSize);

		if (pIoStackLocation->Parameters.Create.SecurityContext->AccessState) {
			// Copy security context
			eventContext->Operation.Create.SecurityContext.AccessState
				.SecurityEvaluated = pIoStackLocation->Parameters.Create.SecurityContext
				->AccessState->SecurityEvaluated;
			eventContext->Operation.Create.SecurityContext.AccessState.GenerateAudit =
				pIoStackLocation->Parameters.Create.SecurityContext->AccessState->GenerateAudit;
			eventContext->Operation.Create.SecurityContext.AccessState
				.GenerateOnClose = pIoStackLocation->Parameters.Create.SecurityContext
				->AccessState->GenerateOnClose;
			eventContext->Operation.Create.SecurityContext.AccessState
				.AuditPrivileges = pIoStackLocation->Parameters.Create.SecurityContext
				->AccessState->AuditPrivileges;
			eventContext->Operation.Create.SecurityContext.AccessState.Flags =
				pIoStackLocation->Parameters.Create.SecurityContext->AccessState->Flags;
			eventContext->Operation.Create.SecurityContext.AccessState
				.RemainingDesiredAccess = pIoStackLocation->Parameters.Create.SecurityContext
				->AccessState->RemainingDesiredAccess;
			eventContext->Operation.Create.SecurityContext.AccessState
				.PreviouslyGrantedAccess = pIoStackLocation->Parameters.Create.SecurityContext
				->AccessState->PreviouslyGrantedAccess;
			eventContext->Operation.Create.SecurityContext.AccessState
				.OriginalDesiredAccess = pIoStackLocation->Parameters.Create.SecurityContext
				->AccessState->OriginalDesiredAccess;

			// NOTE: AccessState offsets are relative to the start address of
			// AccessState

			if (securityDescriptorSize > 0) {
				eventContext->Operation.Create.SecurityContext.AccessState
					.SecurityDescriptorOffset =
					(ULONG)(((char *)eventContext + alignedEventContextSize) -
					(char *)&eventContext->Operation.Create.SecurityContext
						.AccessState);
			}

			eventContext->Operation.Create.SecurityContext.AccessState
				.UnicodeStringObjectNameOffset = (ULONG)(
				((char *)eventContext + alignedEventContextSize +
					securityDescriptorSize) -
					(char *)&eventContext->Operation.Create.SecurityContext.AccessState);
			eventContext->Operation.Create.SecurityContext.AccessState
				.UnicodeStringObjectTypeOffset =
				eventContext->Operation.Create.SecurityContext.AccessState
				.UnicodeStringObjectNameOffset +
				alignedObjectNameSize;
		}

		// Other SecurityContext attributes
		eventContext->Operation.Create.SecurityContext.DesiredAccess =
			pIoStackLocation->Parameters.Create.SecurityContext->DesiredAccess;

		// Other Create attributes
		eventContext->Operation.Create.FileAttributes =
			pIoStackLocation->Parameters.Create.FileAttributes;
		eventContext->Operation.Create.CreateOptions =
			pIoStackLocation->Parameters.Create.Options;
		if (IS_DEVICE_READ_ONLY(DeviceObject) && // do not reorder eval as
			CreateDisposition == FILE_OPEN_IF) {
			// Substitute FILE_OPEN for FILE_OPEN_IF
			// We check on return from userland in DokanCompleteCreate below
			// and if file didn't exist, return write-protected status
			eventContext->Operation.Create.CreateOptions &=
				((FILE_OPEN << 24) | 0x00FFFFFF);
		}
		eventContext->Operation.Create.ShareAccess =
			pIoStackLocation->Parameters.Create.ShareAccess;
		eventContext->Operation.Create.FileNameLength =
			parentDir ? fileNameLength : fcb->FileName.Length;
		eventContext->Operation.Create.FileNameOffset =
			(ULONG)(((char *)eventContext + alignedEventContextSize +
				securityDescriptorSize + alignedObjectNameSize +
				alignedObjectTypeNameSize) -
				(char *)&eventContext->Operation.Create);

		if (newFileSecurityDescriptor != NULL) {
			// Copy security descriptor
			RtlCopyMemory((char *)eventContext + alignedEventContextSize,
				newFileSecurityDescriptor,
				RtlLengthSecurityDescriptor(newFileSecurityDescriptor));
			SeDeassignSecurity(&newFileSecurityDescriptor);
			newFileSecurityDescriptor = NULL;
		}

		if (pIoStackLocation->Parameters.Create.SecurityContext->AccessState) {
			// Object name
			intermediateUnicodeStr = (PDOKAN_UNICODE_STRING_INTERMEDIATE)(
				(char *)&eventContext->Operation.Create.SecurityContext.AccessState +
				eventContext->Operation.Create.SecurityContext.AccessState
				.UnicodeStringObjectNameOffset);
			intermediateUnicodeStr->Length = pIoStackLocation->Parameters.Create.SecurityContext
				->AccessState->ObjectName.Length;
			intermediateUnicodeStr->MaximumLength = (USHORT)alignedObjectNameSize;

			if (pIoStackLocation->Parameters.Create.SecurityContext->AccessState->ObjectName
				.Length > 0) {

				RtlCopyMemory(intermediateUnicodeStr->Buffer,
					pIoStackLocation->Parameters.Create.SecurityContext->AccessState
					->ObjectName.Buffer,
					pIoStackLocation->Parameters.Create.SecurityContext->AccessState
					->ObjectName.Length);

				*(WCHAR *)((char *)intermediateUnicodeStr->Buffer +
					intermediateUnicodeStr->Length) = 0;
			}
			else {
				intermediateUnicodeStr->Buffer[0] = 0;
			}

			// Object type name
			intermediateUnicodeStr = (PDOKAN_UNICODE_STRING_INTERMEDIATE)(
				(char *)intermediateUnicodeStr + alignedObjectNameSize);
			intermediateUnicodeStr->Length = pIoStackLocation->Parameters.Create.SecurityContext
				->AccessState->ObjectTypeName.Length;
			intermediateUnicodeStr->MaximumLength = (USHORT)alignedObjectTypeNameSize;

			if (pIoStackLocation->Parameters.Create.SecurityContext->AccessState->ObjectTypeName
				.Length > 0) {

				RtlCopyMemory(intermediateUnicodeStr->Buffer,
					pIoStackLocation->Parameters.Create.SecurityContext->AccessState
					->ObjectTypeName.Buffer,
					pIoStackLocation->Parameters.Create.SecurityContext->AccessState
					->ObjectTypeName.Length);

				*(WCHAR *)((char *)intermediateUnicodeStr->Buffer +
					intermediateUnicodeStr->Length) = 0;
			}
			else {
				intermediateUnicodeStr->Buffer[0] = 0;
			}
		}

		// other context info
		eventContext->Context = 0;
		eventContext->FileFlags |= fcb->Flags;

		// copy the file name

		RtlCopyMemory(((char *)&eventContext->Operation.Create +
			eventContext->Operation.Create.FileNameOffset),
			parentDir ? fileName : fcb->FileName.Buffer,
			parentDir ? fileNameLength : fcb->FileName.Length);
		*(PWCHAR)((char *)&eventContext->Operation.Create +
			eventContext->Operation.Create.FileNameOffset +
			(parentDir ? fileNameLength : fcb->FileName.Length)) = 0;

		//
		// Oplock
		//

#if (NTDDI_VERSION >= NTDDI_WIN7)
		OpenRequiringOplock = BooleanFlagOn(pIoStackLocation->Parameters.Create.Options,
			FILE_OPEN_REQUIRING_OPLOCK);
#else
		OpenRequiringOplock = FALSE;
#endif

		// Share access support

		if (fcb->FileCount > 1) {

			//
			//  Check if the Fcb has the proper share access.  This routine will
			//  also
			//  check for writable user sections if the user did not allow write
			//  sharing.
			//

			// DokanCheckShareAccess() will update the share access in the fcb if
			// the operation is allowed to go forward

			ntStatus = DokanCheckShareAccess(
				pFileObject, fcb,
				eventContext->Operation.Create.SecurityContext.DesiredAccess,
				eventContext->Operation.Create.ShareAccess);

			if (!NT_SUCCESS(ntStatus)) {

				DDbgPrint("   DokanCheckShareAccess failed with 0x%x\n", ntStatus);

#if (NTDDI_VERSION >= NTDDI_WIN7)

				NTSTATUS OplockBreakStatus = STATUS_SUCCESS;

				//
				//  If we got a sharing violation try to break outstanding
				//  handle
				//  oplocks and retry the sharing check.  If the caller
				//  specified
				//  FILE_COMPLETE_IF_OPLOCKED we don't bother breaking the
				//  oplock;
				//  we just return the sharing violation.
				//
				if ((ntStatus == STATUS_SHARING_VIOLATION) &&
					!FlagOn(pIoStackLocation->Parameters.Create.Options,
						FILE_COMPLETE_IF_OPLOCKED)) {

					OplockBreakStatus = FsRtlOplockBreakH(
						DokanGetFcbOplock(fcb), Irp, 0, eventContext,
						NULL /* DokanOplockComplete */, // block instead of callback
						DokanPrePostIrp);

					//
					//  If FsRtlOplockBreakH returned STATUS_PENDING,
					//  then the IRP
					//  has been posted and we need to stop working.
					//
					if (OplockBreakStatus == STATUS_PENDING) { // shouldn't happen now
						DDbgPrint("   FsRtlOplockBreakH returned STATUS_PENDING\n");
						ntStatus = STATUS_PENDING;
						__leave;
						//
						//  If FsRtlOplockBreakH returned an error
						//  we want to return that now.
						//
					}
					else if (!NT_SUCCESS(OplockBreakStatus)) {
						DDbgPrint("   FsRtlOplockBreakH returned 0x%x\n",
							OplockBreakStatus);
						ntStatus = OplockBreakStatus;
						__leave;

						//
						//  Otherwise FsRtlOplockBreakH returned
						//  STATUS_SUCCESS, indicating
						//  that there is no oplock to be broken.
						//  The sharing violation is
						//  returned in that case.
						//
						//  We actually now pass null for the callback to
						//  FsRtlOplockBreakH so it will block until
						//  the oplock break is sent to holder of the oplock.
						//  This doesn't necessarily mean that the
						//  resourc was freed (file was closed) yet,
						//  but we check share access again in case it was
						//  to see if we can proceed normally or if we
						//  still have to reutrn a sharing violation.
					}
					else {
						ntStatus = DokanCheckShareAccess(
							pFileObject, fcb,
							eventContext->Operation.Create.SecurityContext.DesiredAccess,
							eventContext->Operation.Create.ShareAccess);
						DDbgPrint("    checked share access again, status = 0x%08x\n",
							ntStatus);
						NT_ASSERT(OplockBreakStatus == STATUS_SUCCESS);
						if (ntStatus != STATUS_SUCCESS)
							__leave;
					}

					//
					//  The initial sharing check failed with something
					//  other than sharing
					//  violation (which should never happen, but let's
					//  be future-proof),
					//  or we *did* get a sharing violation and the
					//  caller specified
					//  FILE_COMPLETE_IF_OPLOCKED.  Either way this
					//  create is over.
					//
					// We can't really handle FILE_COMPLETE_IF_OPLOCKED correctly because
					// we don't have a way of creating a useable file handle
					// without actually creating the file in user mode, which
					// won't work unless the oplock gets broken before the
					// user mode create happens.
					// It is believed that FILE_COMPLETE_IF_OPLOCKED is extremely
					// rare and may never happend during normal operation.

				}
				else {

					if (ntStatus == STATUS_SHARING_VIOLATION &&
						FlagOn(pIoStackLocation->Parameters.Create.Options,
							FILE_COMPLETE_IF_OPLOCKED)) {
						DDbgPrint("failing a create with FILE_COMPLETE_IF_OPLOCKED because "
							"of sharing violation\n");
					}

					DDbgPrint("create: sharing/oplock failed, status = 0x%08x\n", ntStatus);
					__leave;
				}

#else
				return ntStatus;
#endif
			}
			ExAcquireResourceExclusiveLite(&fcb->Resource, TRUE);
			IoUpdateShareAccess(pFileObject, &fcb->ShareAccess);
			ExReleaseResourceLite(&fcb->Resource);

		}
		else {
			ExAcquireResourceExclusiveLite(&fcb->Resource, TRUE);
			IoSetShareAccess(
				eventContext->Operation.Create.SecurityContext.DesiredAccess,
				eventContext->Operation.Create.ShareAccess, pFileObject,
				&fcb->ShareAccess);
			ExReleaseResourceLite(&fcb->Resource);
		}

		UnwindShareAccess = TRUE;

		//  Now check that we can continue based on the oplock state of the
		//  file.  If there are no open handles yet in addition to this new one
		//  we don't need to do this check; oplocks can only exist when there are
		//  handles.
		//
		//  It is important that we modified the DesiredAccess in place so
		//  that the Oplock check proceeds against any added access we had
		//  to give the caller.
		//
		if (fcb->FileCount > 1) {
			ntStatus =
				FsRtlCheckOplock(DokanGetFcbOplock(fcb), Irp, eventContext,
					NULL /* DokanOplockComplete */, DokanPrePostIrp);

			//
			//  if FsRtlCheckOplock returns STATUS_PENDING the IRP has been posted
			//  to service an oplock break and we need to leave now.
			//
			if (ntStatus == STATUS_PENDING) {
				DDbgPrint("   FsRtlCheckOplock returned STATUS_PENDING, fileName = "
					"%wZ, fileCount = %lu\n",
					fcb->FileName, fcb->FileCount);
				__leave;
			}
		}

		if (OpenRequiringOplock) {
			DDbgPrint("   OpenRequiringOplock\n");
			//
			//  If the caller wants atomic create-with-oplock semantics, tell
			//  the oplock package.
			if ((ntStatus == STATUS_SUCCESS)) {
				ntStatus = FsRtlOplockFsctrl(DokanGetFcbOplock(fcb), Irp, fcb->FileCount);
			}

			//
			//  If we've encountered a failure we need to leave.  FsRtlCheckOplock
			//  will have returned STATUS_OPLOCK_BREAK_IN_PROGRESS if it initiated
			//  and oplock break and the caller specified FILE_COMPLETE_IF_OPLOCKED
			//  on the create call.  That's an NT_SUCCESS code, so we need to keep
			//  going.
			//
			if ((ntStatus != STATUS_SUCCESS) &&
				(ntStatus != STATUS_OPLOCK_BREAK_IN_PROGRESS)) {
				DDbgPrint("   FsRtlOplockFsctrl failed with 0x%x, fileName = %wZ, "
					"fileCount = %lu\n",
					ntStatus, fcb->FileName, fcb->FileCount);

				__leave;
			}
			else if (ntStatus == STATUS_OPLOCK_BREAK_IN_PROGRESS) {
				DDbgPrint("create: STATUS_OPLOCK_BREAK_IN_PROGRESS\n");
			}
			// if we fail after this point, the oplock will need to be backed out
			// if the oplock was granted (status == STATUS_SUCCESS)
			if (ntStatus == STATUS_SUCCESS) {
				BackoutOplock = TRUE;
			}
		}

		// register this IRP to waiting IPR list
		ntStatus = DokanRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

		EventContextConsumed = TRUE;

	}
	__finally
	{

		DDbgPrint("  Create: FileName:%wZ, status = 0x%08x\n",
			&pFileObject->FileName, ntStatus);

		// Getting here by __leave isn't always a failure,
		// so we shouldn't necessarily clean up only because
		// AbnormalTermination() returns true

#if (NTDDI_VERSION >= NTDDI_WIN7)
	//
	//  If we're not getting out with success, and if the caller wanted
	//  atomic create-with-oplock semantics make sure we back out any
	//  oplock that may have been granted.
	//
	//  Also unwind any share access that was added to the fcb

		if (!NT_SUCCESS(ntStatus)) {
			if (BackoutOplock) {
				FsRtlCheckOplockEx(DokanGetFcbOplock(fcb), Irp,
					OPLOCK_FLAG_BACK_OUT_ATOMIC_OPLOCK, NULL, NULL,
					NULL);
			}

			if (UnwindShareAccess) {
				ExAcquireResourceExclusiveLite(&fcb->Resource, TRUE);
				IoRemoveShareAccess(pFileObject, &fcb->ShareAccess);
				ExReleaseResourceLite(&fcb->Resource);
			}
		}
#endif

		if (!NT_SUCCESS(ntStatus)) {

			// DokanRegisterPendingIrp consumes event context

			if (!EventContextConsumed && eventContext) {
				DokanFreeEventContext(eventContext);
			}
			if (ccb) {
				DokanFreeCCB(ccb);
			}
			if (fcb) {
				DokanFreeFCB(fcb);
			}
		}

		if (parentDir) { // SL_OPEN_TARGET_DIRECTORY
						 // fcb owns parentDir, not fileName
			if (fileName)
				ExFreePool(fileName);
		}

		DokanCompleteIrpRequest(Irp, ntStatus, info);

		DDbgPrint("<== DokanCreate\n");
	}

	return ntStatus;
}
