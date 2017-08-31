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
#include "QueryInformation.h"

NTSTATUS
DokanDispatchQueryInformation(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	PIO_STACK_LOCATION irpSp;
	PFILE_OBJECT fileObject;
	PDokanCCB ccb;
	PDokanFCB fcb;
	PDokanVCB vcb;
	ULONG info = 0;
	ULONG eventLength;
	PEVENT_CONTEXT eventContext;

	// PAGED_CODE();

	__try {
		DDbgPrint("==> DokanQueryInformation\n");

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;

		DDbgPrint("  FileInfoClass %d\n",
			irpSp->Parameters.QueryFile.FileInformationClass);
		DDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));

		if (fileObject == NULL) {
			DDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		DokanPrintFileName(fileObject);

		/*
		if (fileObject->FsContext2 == NULL &&
		fileObject->FileName.Length == 0) {
		// volume open?
		status = STATUS_SUCCESS;
		__leave;
		}*/
		vcb = (PDokanVCB)DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB ||
			!DokanCheckCCB(vcb->Dcb, (PDokanCCB)fileObject->FsContext2)) {
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		ccb = (PDokanCCB)fileObject->FsContext2;
		ASSERT(ccb != NULL);

		fcb = ccb->Fcb;
		ASSERT(fcb != NULL);

		switch (irpSp->Parameters.QueryFile.FileInformationClass) {
		case FileBasicInformation:
			DDbgPrint("  FileBasicInformation\n");
			break;
		case FileInternalInformation:
			DDbgPrint("  FileInternalInformation\n");
			break;
		case FileEaInformation:
			DDbgPrint("  FileEaInformation\n");
			break;
		case FileStandardInformation:
			DDbgPrint("  FileStandardInformation\n");
			break;
		case FileAllInformation:
			DDbgPrint("  FileAllInformation\n");
			break;
		case FileAlternateNameInformation:
			DDbgPrint("  FileAlternateNameInformation\n");
			break;
		case FileAttributeTagInformation:
			DDbgPrint("  FileAttributeTagInformation\n");
			break;
		case FileCompressionInformation:
			DDbgPrint("  FileCompressionInformation\n");
			break;
		case FileNormalizedNameInformation: // Fake implementation by returning
											// FileNameInformation result.
											// TODO: implement it
			DDbgPrint("  FileNormalizedNameInformation\n");
		case FileNameInformation: {
			PFILE_NAME_INFORMATION nameInfo;

			DDbgPrint("  FileNameInformation\n");

			if (irpSp->Parameters.QueryFile.Length <
				sizeof(FILE_NAME_INFORMATION) + fcb->FileName.Length) {

				info = irpSp->Parameters.QueryFile.Length;
				status = STATUS_BUFFER_OVERFLOW;

			}
			else {

				nameInfo = (PFILE_NAME_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
				ASSERT(nameInfo != NULL);

				nameInfo->FileNameLength = fcb->FileName.Length;
				RtlCopyMemory(nameInfo->FileName, fcb->FileName.Buffer,
					fcb->FileName.Length);
				info = FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) +
					fcb->FileName.Length;
				status = STATUS_SUCCESS;
			}
			__leave;
		} break;
		case FileNetworkOpenInformation:
			DDbgPrint("  FileNetworkOpenInformation\n");
			break;
		case FilePositionInformation: {
			PFILE_POSITION_INFORMATION posInfo;

			DDbgPrint("  FilePositionInformation\n");

			if (irpSp->Parameters.QueryFile.Length <
				sizeof(FILE_POSITION_INFORMATION)) {
				status = STATUS_INFO_LENGTH_MISMATCH;

			}
			else {
				posInfo = (PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
				ASSERT(posInfo != NULL);

				RtlZeroMemory(posInfo, sizeof(FILE_POSITION_INFORMATION));

				if (fileObject->CurrentByteOffset.QuadPart < 0) {
					status = STATUS_INVALID_PARAMETER;
				}
				else {
					// set the current file offset
					posInfo->CurrentByteOffset = fileObject->CurrentByteOffset;

					info = sizeof(FILE_POSITION_INFORMATION);
					status = STATUS_SUCCESS;
				}
			}
			__leave;
		} break;
		case FileStreamInformation:
			DDbgPrint("  FileStreamInformation\n");
			break;
		case FileStandardLinkInformation:
			DDbgPrint("  FileStandardLinkInformation\n");
			break;
		case FileNetworkPhysicalNameInformation:
			DDbgPrint("  FileNetworkPhysicalNameInformation\n");
			break;
		case FileRemoteProtocolInformation:
			DDbgPrint("  FileRemoteProtocolInformation\n");
			break;
		default:
			DDbgPrint("  unknown type:%d\n",
				irpSp->Parameters.QueryFile.FileInformationClass);
			break;
		}

		// if it is not treadted in swich case

		// calculate the length of EVENT_CONTEXT
		// sum of it's size and file name length
		eventLength = sizeof(EVENT_CONTEXT) + fcb->FileName.Length;

		eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);

		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		eventContext->Context = ccb->UserContext;
		// DDbgPrint("   get Context %X\n", (ULONG)ccb->UserContext);

		eventContext->Operation.File.FileInformationClass =
			irpSp->Parameters.QueryFile.FileInformationClass;

		// bytes length which is able to be returned
		eventContext->Operation.File.BufferLength =
			irpSp->Parameters.QueryFile.Length;

		// copy file name to EventContext from FCB
		eventContext->Operation.File.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->Operation.File.FileName, fcb->FileName.Buffer,
			fcb->FileName.Length);

		// register this IRP to pending IPR list
		status = DokanRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

	}
	__finally {

		DokanCompleteIrpRequest(Irp, status, info);

		DDbgPrint("<== DokanQueryInformation\n");
	}

	return status;
}

VOID DokanCompleteQueryInformation(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo) {
	PIRP irp;
	PIO_STACK_LOCATION irpSp;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG info = 0;
	ULONG bufferLen = 0;
	PVOID buffer = NULL;
	PDokanCCB ccb;

	DDbgPrint("==> DokanCompleteQueryInformation\n");

	irp = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;

	ccb = (PDokanCCB)IrpEntry->FileObject->FsContext2;

	ASSERT(ccb != NULL);

	ccb->UserContext = EventInfo->Context;
	// DDbgPrint("   set Context %X\n", (ULONG)ccb->UserContext);

	// where we shold copy FileInfo to
	buffer = irp->AssociatedIrp.SystemBuffer;

	// available buffer size
	bufferLen = irpSp->Parameters.QueryFile.Length;

	// buffer is not specifed or short of size
	if (bufferLen == 0 || buffer == NULL || bufferLen < EventInfo->BufferLength) {
		info = 0;
		status = STATUS_INSUFFICIENT_RESOURCES;

	}
	else {

		//
		// we write FileInfo from user mode
		//
		ASSERT(buffer != NULL);

		RtlZeroMemory(buffer, bufferLen);
		RtlCopyMemory(buffer, EventInfo->Buffer, EventInfo->BufferLength);

		// written bytes
		info = EventInfo->BufferLength;
		status = EventInfo->Status;

		if (NT_SUCCESS(status) &&
			irpSp->Parameters.QueryFile.FileInformationClass ==
			FileAllInformation) {

			PFILE_ALL_INFORMATION allInfo = (PFILE_ALL_INFORMATION)buffer;
			allInfo->PositionInformation.CurrentByteOffset =
				IrpEntry->FileObject->CurrentByteOffset;
		}
	}

	DokanCompleteIrpRequest(irp, status, info);

	DDbgPrint("<== DokanCompleteQueryInformation\n");
}
