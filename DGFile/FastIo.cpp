
#include "Stdafx.h"
#include "FastIo.h"

BOOLEAN
DokanFastIoCheckIfPossible(__in PFILE_OBJECT FileObject,
	__in PLARGE_INTEGER FileOffset, __in ULONG Length,
	__in BOOLEAN Wait, __in ULONG LockKey,
	__in BOOLEAN CheckForReadOperation,
	__out PIO_STATUS_BLOCK IoStatus,
	__in PDEVICE_OBJECT DeviceObject) {
	UNREFERENCED_PARAMETER(FileObject);
	UNREFERENCED_PARAMETER(FileOffset);
	UNREFERENCED_PARAMETER(Length);
	UNREFERENCED_PARAMETER(Wait);
	UNREFERENCED_PARAMETER(LockKey);
	UNREFERENCED_PARAMETER(CheckForReadOperation);
	UNREFERENCED_PARAMETER(IoStatus);
	UNREFERENCED_PARAMETER(DeviceObject);

	DDbgPrint("DokanFastIoCheckIfPossible\n");
	return FALSE;
}

BOOLEAN
DokanFastIoRead(__in PFILE_OBJECT FileObject, __in PLARGE_INTEGER FileOffset,
	__in ULONG Length, __in BOOLEAN Wait, __in ULONG LockKey,
	__in PVOID Buffer, __out PIO_STATUS_BLOCK IoStatus,
	__in PDEVICE_OBJECT DeviceObject) {
	UNREFERENCED_PARAMETER(FileObject);
	UNREFERENCED_PARAMETER(FileOffset);
	UNREFERENCED_PARAMETER(Length);
	UNREFERENCED_PARAMETER(Wait);
	UNREFERENCED_PARAMETER(LockKey);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(IoStatus);
	UNREFERENCED_PARAMETER(DeviceObject);

	DDbgPrint("DokanFastIoRead\n");
	return FALSE;
}

VOID DokanAcquireForCreateSection(__in PFILE_OBJECT FileObject) {
	PFSRTL_ADVANCED_FCB_HEADER header;

	header = (PFSRTL_ADVANCED_FCB_HEADER)FileObject->FsContext;
	if (header && header->Resource) {
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(header->Resource, TRUE);
		KeLeaveCriticalRegion();
	}

	DDbgPrint("DokanAcquireForCreateSection\n");
}

VOID DokanReleaseForCreateSection(__in PFILE_OBJECT FileObject) {
	PFSRTL_ADVANCED_FCB_HEADER header;

	header = (PFSRTL_ADVANCED_FCB_HEADER)FileObject->FsContext;
	if (header && header->Resource) {
		KeEnterCriticalRegion();
		ExReleaseResourceLite(header->Resource);
		KeLeaveCriticalRegion();
	}

	DDbgPrint("DokanReleaseForCreateSection\n");
}

NTSTATUS
DokanFilterCallbackAcquireForCreateSection(__in PFS_FILTER_CALLBACK_DATA
	CallbackData,
	__out PVOID *CompletionContext) {
	PFSRTL_ADVANCED_FCB_HEADER header;

	UNREFERENCED_PARAMETER(CompletionContext);

	DDbgPrint("DokanFilterCallbackAcquireForCreateSection\n");

	header = (PFSRTL_ADVANCED_FCB_HEADER)CallbackData->FileObject->FsContext;

	if (header && header->Resource) {
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(header->Resource, TRUE);
		KeLeaveCriticalRegion();
	}

	if (CallbackData->Parameters.AcquireForSectionSynchronization.SyncType !=
		SyncTypeCreateSection) {
		return STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY;
	}
	else {
		return STATUS_FILE_LOCKED_WITH_WRITERS;
	}
}
