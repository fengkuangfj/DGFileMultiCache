#pragma once

#define MDL_TAG 'TLDM' // MDLT

NTSTATUS
DokanAllocateMdl(__in PIRP Irp, __in ULONG Length);

VOID DokanFreeMdl(__in PIRP Irp);
