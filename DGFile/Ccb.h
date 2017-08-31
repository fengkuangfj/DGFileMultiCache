
#pragma once

#define CCB_TAG 'TBCC' // CCBT

PDokanCCB DokanAllocateCCB(__in PDokanDCB Dcb, __in PDokanFCB Fcb);

NTSTATUS
DokanFreeCCB(__in PDokanCCB ccb);

BOOLEAN
DokanCheckCCB(__in PDokanDCB Dcb, __in_opt PDokanCCB Ccb);
