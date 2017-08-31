
#pragma once

#define WRITE_TAG 'TIRW' // WRIT


VOID DokanCompleteWrite(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);

NTSTATUS
DokanDispatchWrite(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

