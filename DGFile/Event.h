
#pragma once

#define EVENT_TAG 'NEVE' // EVEN


PEVENT_CONTEXT
AllocateEventContextRaw(__in ULONG EventContextLength);

PEVENT_CONTEXT
AllocateEventContext(__in PDokanDCB Dcb, __in PIRP Irp,
	__in ULONG EventContextLength, __in_opt PDokanCCB Ccb);

VOID DokanFreeEventContext(__in PEVENT_CONTEXT EventContext);


NTSTATUS DokanEventRelease(__in PDEVICE_OBJECT DeviceObject);



NTSTATUS DokanGlobalEventRelease(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp);