
#pragma once

#define QUERY_VOLUME_INFORMATION_TAG 'IVUQ' // QUVI

VOID DokanCompleteQueryVolumeInformation(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo,
	__in PDEVICE_OBJECT DeviceObject);

NTSTATUS
DokanDispatchQueryVolumeInformation(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp);
