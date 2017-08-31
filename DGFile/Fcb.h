
#pragma once

#define FCB_TAG 'TBCF' // FCBT

PDokanFCB DokanAllocateFCB(__in PDokanVCB Vcb);

NTSTATUS
DokanFreeFCB(__in PDokanFCB Fcb);

PDokanFCB DokanGetFCB(__in PDokanVCB Vcb, __in PWCHAR FileName,
	__in ULONG FileNameLength);
