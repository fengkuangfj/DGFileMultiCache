
#pragma once

#define SET_SERURITY_TAG 'ESES' // SESE

VOID DokanCompleteSetSecurity(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);

NTSTATUS
DokanDispatchSetSecurity(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
