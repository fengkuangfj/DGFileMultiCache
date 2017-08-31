
#pragma once

#define DIRECTORY_CONTROL_TAG 'OCID' // DICO

NTSTATUS
DokanDispatchDirectoryControl(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanQueryDirectory(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanNotifyChangeDirectory(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

VOID DokanCompleteDirectoryControl(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);
