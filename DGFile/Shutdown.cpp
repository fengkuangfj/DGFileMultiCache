
#include "Stdafx.h"
#include "Shutdown.h"

NTSTATUS
DokanDispatchShutdown(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	// PAGED_CODE();
	DDbgPrint("==> DokanShutdown\n");

	DokanCompleteIrpRequest(Irp, STATUS_SUCCESS, 0);

	DDbgPrint("<== DokanShutdown\n");
	return STATUS_SUCCESS;
}
