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
#include "SetSecurity.h"


NTSTATUS
DokanDispatchSetSecurity(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	PIO_STACK_LOCATION irpSp;
	PDokanVCB vcb;
	PDokanDCB dcb;
	PDokanCCB ccb;
	PDokanFCB fcb;
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	PFILE_OBJECT fileObject;
	ULONG info = 0;
	PSECURITY_INFORMATION securityInfo;
	PSECURITY_DESCRIPTOR securityDescriptor;
	ULONG securityDescLength;
	ULONG eventLength;
	PEVENT_CONTEXT eventContext;

	__try {
		DDbgPrint("==> DokanSetSecurity\n");

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
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		securityInfo = &irpSp->Parameters.SetSecurity.SecurityInformation;

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

		securityDescriptor = irpSp->Parameters.SetSecurity.SecurityDescriptor;

		// Assumes the parameter is self relative SD.
		securityDescLength = RtlLengthSecurityDescriptor(securityDescriptor);

		eventLength =
			sizeof(EVENT_CONTEXT) + securityDescLength + fcb->FileName.Length;

		if (EVENT_CONTEXT_MAX_SIZE < eventLength) {
			// TODO: Handle this case like DispatchWrite.
			DDbgPrint("    SecurityDescriptor is too big: %d (limit %d)\n",
				eventLength, EVENT_CONTEXT_MAX_SIZE);
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);

		if (eventContext == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		eventContext->Context = ccb->UserContext;
		eventContext->Operation.SetSecurity.SecurityInformation = *securityInfo;
		eventContext->Operation.SetSecurity.BufferLength = securityDescLength;
		eventContext->Operation.SetSecurity.BufferOffset =
			FIELD_OFFSET(EVENT_CONTEXT, Operation.SetSecurity.FileName[0]) +
			fcb->FileName.Length + sizeof(WCHAR);
		RtlCopyMemory((PCHAR)eventContext +
			eventContext->Operation.SetSecurity.BufferOffset,
			securityDescriptor, securityDescLength);

		eventContext->Operation.SetSecurity.FileNameLength = fcb->FileName.Length;
		RtlCopyMemory(eventContext->Operation.SetSecurity.FileName,
			fcb->FileName.Buffer, fcb->FileName.Length);

		status = DokanRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

	}
	__finally {

		DokanCompleteIrpRequest(Irp, status, info);

		DDbgPrint("<== DokanSetSecurity\n");
	}

	return status;
}

VOID DokanCompleteSetSecurity(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo) {
	PIRP irp;
	PIO_STACK_LOCATION irpSp;
	PFILE_OBJECT fileObject;
	PDokanCCB ccb;

	DDbgPrint("==> DokanCompleteSetSecurity\n");

	irp = IrpEntry->Irp;
	irpSp = IrpEntry->IrpSp;

	fileObject = IrpEntry->FileObject;
	ASSERT(fileObject != NULL);

	ccb = (PDokanCCB)fileObject->FsContext2;
	if (ccb != NULL) {
		ccb->UserContext = EventInfo->Context;
	}
	else {
		DDbgPrint("  ccb == NULL\n");
	}

	DokanCompleteIrpRequest(irp, EventInfo->Status, 0);

	DDbgPrint("<== DokanCompleteSetSecurity\n");
}