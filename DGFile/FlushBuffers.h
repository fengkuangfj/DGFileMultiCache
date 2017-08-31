
#pragma once

#define FLUSH_BUFFERS_TAG 'UBLF' // FLBU



VOID DokanCompleteFlush(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);

NTSTATUS
DokanDispatchFlush(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

