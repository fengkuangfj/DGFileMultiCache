
#pragma once

#define CLEANUP_TAG 'LELC' // CLEA

NTSTATUS
DokanDispatchCleanup(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

VOID DokanCompleteCleanup(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);
