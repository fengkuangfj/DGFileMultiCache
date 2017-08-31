
#pragma once

#define  LOCK_CONTROL_TAG 'OCOL' // LOCO

VOID DokanCompleteLock(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);

NTSTATUS
DokanDispatchLock(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanCommonLockControl(__in PIRP Irp, __in PEVENT_CONTEXT EventContext);
