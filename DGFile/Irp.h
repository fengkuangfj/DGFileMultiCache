
#pragma once

#define IRP_TAG 'TPRI' // IRPT



VOID DokanIrpCancelRoutine(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

VOID DokanPrePostIrp(IN PVOID Context, IN PIRP Irp);

NTSTATUS
RegisterPendingIrpMain(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in ULONG SerialNumber, __in PIRP_LIST IrpList,
	__in ULONG Flags, __in ULONG CheckMount);

NTSTATUS
DokanRegisterPendingIrp(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in PEVENT_CONTEXT EventContext, __in ULONG Flags);

NTSTATUS
DokanRegisterPendingIrpForEvent(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp);

NTSTATUS
DokanRegisterPendingIrpForService(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp);


NTSTATUS
DokanCompleteIrp(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);