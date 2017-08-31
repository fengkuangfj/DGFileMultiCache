
#pragma once

#define READ_TAG 'DAER' // READ


VOID DokanCompleteRead(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);


NTSTATUS
DokanDispatchRead(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
