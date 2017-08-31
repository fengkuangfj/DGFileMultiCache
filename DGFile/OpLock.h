
#pragma once

#define OP_LOCK_TAG 'OLPO' // OPLO

VOID DokanOplockComplete(IN PVOID Context, IN PIRP Irp);

NTSTATUS DokanOplockRequest(__in PIRP *pIrp);
