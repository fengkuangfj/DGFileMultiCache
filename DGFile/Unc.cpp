
#include "Stdafx.h"
#include "Unc.h"

VOID DokanRegisterUncProvider(__in PDokanDCB Dcb) {
	NTSTATUS status;

	if (Dcb->UNCName != NULL && Dcb->UNCName->Length > 0) {
		status =
			FsRtlRegisterUncProvider(&(Dcb->MupHandle), Dcb->DiskDeviceName, FALSE);
		if (NT_SUCCESS(status)) {
			DDbgPrint("  FsRtlRegisterUncProvider success\n");
		}
		else {
			DDbgPrint("  FsRtlRegisterUncProvider failed: 0x%x\n", status);
			Dcb->MupHandle = 0;
		}
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS DokanRegisterUncProviderSystem(PDokanDCB dcb) {
	// Run FsRtlRegisterUncProvider in System thread.
	HANDLE handle;
	PKTHREAD thread;
	OBJECT_ATTRIBUTES objectAttribs;
	NTSTATUS status;

	InitializeObjectAttributes(&objectAttribs, NULL, OBJ_KERNEL_HANDLE, NULL,
		NULL);
	status = PsCreateSystemThread(&handle, THREAD_ALL_ACCESS, &objectAttribs,
		NULL, NULL,
		(PKSTART_ROUTINE)DokanRegisterUncProvider, dcb);
	if (!NT_SUCCESS(status)) {
		DDbgPrint("PsCreateSystemThread failed: 0x%X\n", status);
	}
	else {
		ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, NULL, KernelMode,
			(PVOID *)&thread, NULL);
		ZwClose(handle);
		KeWaitForSingleObject(thread, Executive, KernelMode, FALSE, NULL);
		ObDereferenceObject(thread);
	}
	return status;
}

VOID DokanDeregisterUncProvider(__in PDokanDCB Dcb) {
	if (Dcb->MupHandle) {
		FsRtlDeregisterUncProvider(Dcb->MupHandle);
		Dcb->MupHandle = 0;
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}
