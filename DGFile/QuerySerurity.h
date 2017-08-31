
#pragma once

#define QUERY_SERURITY_TAG 'NIUQ' // QUSE

VOID DokanCompleteQuerySecurity(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);

NTSTATUS
DokanDispatchQuerySecurity(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
