
#pragma once

#define FAST_IO_TAG 'OIAF' // FAIO

BOOLEAN
DokanFastIoCheckIfPossible(__in PFILE_OBJECT FileObject,
	__in PLARGE_INTEGER FileOffset, __in ULONG Length,
	__in BOOLEAN Wait, __in ULONG LockKey,
	__in BOOLEAN CheckForReadOperation,
	__out PIO_STATUS_BLOCK IoStatus,
	__in PDEVICE_OBJECT DeviceObject);

BOOLEAN
DokanFastIoRead(__in PFILE_OBJECT FileObject, __in PLARGE_INTEGER FileOffset,
	__in ULONG Length, __in BOOLEAN Wait, __in ULONG LockKey,
	__in PVOID Buffer, __out PIO_STATUS_BLOCK IoStatus,
	__in PDEVICE_OBJECT DeviceObject);

VOID DokanAcquireForCreateSection(__in PFILE_OBJECT FileObject);

VOID DokanReleaseForCreateSection(__in PFILE_OBJECT FileObject);

NTSTATUS
DokanFilterCallbackAcquireForCreateSection(__in PFS_FILTER_CALLBACK_DATA
	CallbackData,
	__out PVOID *CompletionContext);
