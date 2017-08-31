
#include "Stdafx.h"
#include "DGFile.h"

NTSTATUS
DokanAllocateMdl(__in PIRP Irp, __in ULONG Length) {
	if (Irp->MdlAddress == NULL) {
		Irp->MdlAddress = IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);

		if (Irp->MdlAddress == NULL) {
			DDbgPrint("    IoAllocateMdl returned NULL\n");
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		__try {
			MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, IoWriteAccess);

		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			DDbgPrint("    MmProveAndLockPages error\n");
			IoFreeMdl(Irp->MdlAddress);
			Irp->MdlAddress = NULL;
			return STATUS_INSUFFICIENT_RESOURCES;
		}
	}
	return STATUS_SUCCESS;
}

VOID DokanFreeMdl(__in PIRP Irp) {
	if (Irp->MdlAddress != NULL) {
		MmUnlockPages(Irp->MdlAddress);
		IoFreeMdl(Irp->MdlAddress);
		Irp->MdlAddress = NULL;
	}
}
