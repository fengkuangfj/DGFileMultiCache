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
#include "QuerySerurity.h"

NTSTATUS
DokanDispatchQuerySecurity(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	PIO_STACK_LOCATION irpSp;
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	PFILE_OBJECT fileObject;
	ULONG info = 0;
	ULONG bufferLength;
	PSECURITY_INFORMATION securityInfo;
	PDokanFCB fcb;
	PDokanDCB dcb;
	PDokanVCB vcb;
	PDokanCCB ccb;
	ULONG eventLength;
	PEVENT_CONTEXT eventContext;
	ULONG flags = 0;

	__try {
		DDbgPrint("==> DokanQuerySecurity\n");

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		fileObject = irpSp->FileObject;

		if (fileObject == NULL) {
			DDbgPrint("  fileObject == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		vcb = (PDokanVCB)DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB) {
			DbgPrint("    DeviceExtension != VCB\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		dcb = vcb->Dcb;

		DDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
		DokanPrintFileName(fileObject);

		ccb = (PDokanCCB)fileObject->FsContext2;
		if (ccb == NULL) {
			DDbgPrint("    ccb == NULL\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		fcb = ccb->Fcb;
		if (fcb == NULL) {
			DDbgPrint("    fcb == NULL\n");
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		bufferLength = irpSp->Parameters.QuerySecurity.Length;
		securityInfo = &irpSp->Parameters.QuerySecurity.SecurityInformation;

		if (*securityInfo & OWNER_SECURITY_INFORMATION) {
			DDbgPrint("    OWNER_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & GROUP_SECURITY_INFORMATION) {
			DDbgPrint("    GROUP_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & DACL_SECURITY_INFORMATION) {
			DDbgPrint("    DACL_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & SACL_SECURITY_INFORMATION) {
			DDbgPrint("    SACL_SECURITY_INFORMATION\n");
		}
		if (*securityInfo & LABEL_SECURITY_INFORMATION) {
			DDbgPrint("    LABEL_SECURITY_INFORMATION\n");
		}

		eventLength = sizeof(EVENT_CONTEXT) + fcb->FileName.Length;
		eventContext = AllocateEventContext(dcb, Irp, eventLength, ccb);

		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		if (Irp->UserBuffer != NULL && bufferLength > 0) {
			// make a MDL for UserBuffer that can be used later on another thread
			// context
			if (Irp->MdlAddress == NULL) {
				status = DokanAllocateMdl(Irp, bufferLength);
				if (!NT_SUCCESS(status)) {
					DokanFreeEventContext(eventContext);
					__leave;
				}
				flags = DOKAN_MDL_ALLOCATED;
			}
		}

		eventContext->Context = ccb->UserContext;
		eventContext->Operation.Security.SecurityInformation = *securityInfo;
		eventContext->Operation.Security.BufferLength = bufferLength;

		eventContext->Operation.Security.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->Operation.Security.FileName,
			fcb->FileName.Buffer, fcb->FileName.Length);

		status = DokanRegisterPendingIrp(DeviceObject, Irp, eventContext, flags);

	}
	__finally {

		DokanCompleteIrpRequest(Irp, status, info);

		DDbgPrint("<== DokanQuerySecurity\n");
	}

	return status;
}

VOID DokanCompleteQuerySecurity(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo) {
	PIRP irp;
	PIO_STACK_LOCATION irpSp;
	NTSTATUS status;
	PVOID buffer = NULL;
	ULONG bufferLength;
	ULONG info = 0;
	PFILE_OBJECT fileObject;
	PDokanCCB ccb;

	DDbgPrint("==> DokanCompleteQuerySecurity\n");

	irp = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;

	if (irp->MdlAddress) {
		buffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
	}

	bufferLength = irpSp->Parameters.QuerySecurity.Length;

	if (EventInfo->Status == STATUS_SUCCESS &&
		EventInfo->BufferLength <= bufferLength && buffer != NULL) {
		RtlCopyMemory(buffer, EventInfo->Buffer, EventInfo->BufferLength);
		info = EventInfo->BufferLength;
		status = STATUS_SUCCESS;

	}
	else if (EventInfo->Status == STATUS_BUFFER_OVERFLOW ||
		(EventInfo->Status == STATUS_SUCCESS &&
			bufferLength < EventInfo->BufferLength)) {
		info = EventInfo->BufferLength;
		status = STATUS_BUFFER_OVERFLOW;

	}
	else {
		info = 0;
		status = EventInfo->Status;
	}

	if (IrpEntry->Flags & DOKAN_MDL_ALLOCATED) {
		DokanFreeMdl(irp);
		IrpEntry->Flags &= ~DOKAN_MDL_ALLOCATED;
	}

	fileObject = IrpEntry->FileObject;
	ASSERT(fileObject != NULL);

	ccb = (PDokanCCB)fileObject->FsContext2;
	if (ccb != NULL) {
		ccb->UserContext = EventInfo->Context;
	}
	else {
		DDbgPrint("  ccb == NULL\n");
	}

	DokanCompleteIrpRequest(irp, status, info);

	DDbgPrint("<== DokanCompleteQuerySecurity\n");
}

