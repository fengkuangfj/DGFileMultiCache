
#pragma once

#define QUERY_INFORMATION_TAG 'NIUQ' // QUIN

VOID DokanCompleteQueryInformation(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);

NTSTATUS
DokanDispatchQueryInformation(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
