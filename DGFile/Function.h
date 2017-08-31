
#pragma once

#define FUNCTION_TAG 'CNUF' // FUNC

#define PrintStatus(val, flag)                                                 \
  if (val == flag)                                                             \
  DDbgPrint("  status = " #flag "\n")

NTSTATUS DokanGetParentDir(__in const WCHAR *fileName, __out WCHAR **parentDir,
	__out ULONG *parentDirLength);

LONG DokanUnicodeStringChar(__in PUNICODE_STRING UnicodeString,
	__in WCHAR Char);

NTSTATUS
DokanCheckShareAccess(__in PFILE_OBJECT FileObject, __in PDokanFCB FcbOrDcb,
	__in ACCESS_MASK DesiredAccess, __in ULONG ShareAccess);

NTSTATUS
DokanUserFsRequest(__in PDEVICE_OBJECT DeviceObject, __in PIRP *pIrp);

PUNICODE_STRING
DokanAllocateUnicodeString(__in PCWSTR String);

VOID FreeUnicodeString(PUNICODE_STRING UnicodeString);

ULONG
PointerAlignSize(ULONG sizeInBytes);

VOID DokanPrintNTStatus(NTSTATUS Status);

VOID PrintIdType(__in VOID *Id);

VOID DokanNoOpRelease(__in PVOID Fcb);

BOOLEAN
DokanNoOpAcquire(__in PVOID Fcb, __in BOOLEAN Wait);

VOID DokanNotifyReportChange0(__in PDokanFCB Fcb, __in PUNICODE_STRING FileName,
	__in ULONG FilterMatch, __in ULONG Action);

VOID DokanNotifyReportChange(__in PDokanFCB Fcb, __in ULONG FilterMatch,
	__in ULONG Action);

BOOLEAN
DokanLookasideCreate(LOOKASIDE_LIST_EX *pCache, size_t cbElement);

NTSTATUS
DokanGetAccessToken(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
