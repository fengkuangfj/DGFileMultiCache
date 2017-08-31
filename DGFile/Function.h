
#pragma once

#define FUNCTION_TAG 'CNUF' // FUNC

NTSTATUS DokanGetParentDir(__in const WCHAR *fileName, __out WCHAR **parentDir,
	__out ULONG *parentDirLength);

LONG DokanUnicodeStringChar(__in PUNICODE_STRING UnicodeString,
	__in WCHAR Char);

VOID SetFileObjectForVCB(__in PFILE_OBJECT FileObject, __in PDokanVCB Vcb);

NTSTATUS
DokanCheckShareAccess(__in PFILE_OBJECT FileObject, __in PDokanFCB FcbOrDcb,
	__in ACCESS_MASK DesiredAccess, __in ULONG ShareAccess);

NTSTATUS
DokanUserFsRequest(__in PDEVICE_OBJECT DeviceObject, __in PIRP *pIrp);

PUNICODE_STRING
DokanAllocateUnicodeString(__in PCWSTR String);

VOID FreeUnicodeString(PUNICODE_STRING UnicodeString);
