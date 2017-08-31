
#pragma once

#define CREATE_TAG 'AERC' // CREA




VOID DokanCompleteCreate(__in PIRP_ENTRY IrpEntry,
	__in PEVENT_INFORMATION EventInfo);



NTSTATUS
DokanDispatchCreate(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);


