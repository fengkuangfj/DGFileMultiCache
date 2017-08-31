
#pragma once

#define SET_INFORMATION_TAG 'NIES' // SEIN


VOID DokanCompleteSetInformation(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);

NTSTATUS
DokanDispatchSetInformation(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

