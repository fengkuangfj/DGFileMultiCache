
#include "Stdafx.h"
#include "Ccb.h"

PDokanCCB DokanAllocateCCB(__in PDokanDCB Dcb, __in PDokanFCB Fcb) {
	PDokanCCB ccb = (PDokanCCB)ExAllocateFromLookasideListEx(&g_DokanCCBLookasideList);

	if (ccb == NULL)
		return NULL;

	ASSERT(ccb != NULL);
	ASSERT(Fcb != NULL);

	RtlZeroMemory(ccb, sizeof(DokanCCB));

	ccb->Identifier.Type = CCB;
	ccb->Identifier.Size = sizeof(DokanCCB);

	ccb->Fcb = Fcb;

	ExInitializeResourceLite(&ccb->Resource);

	InitializeListHead(&ccb->NextCCB);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&Fcb->Resource, TRUE);

	InsertTailList(&Fcb->NextCCB, &ccb->NextCCB);

	ExReleaseResourceLite(&Fcb->Resource);
	KeLeaveCriticalRegion();

	ccb->MountId = Dcb->MountId;

	InterlockedIncrement(&Fcb->Vcb->CcbAllocated);
	return ccb;
}

NTSTATUS
DokanFreeCCB(__in PDokanCCB ccb) {
	PDokanFCB fcb;

	ASSERT(ccb != NULL);

	fcb = ccb->Fcb;

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&fcb->Resource, TRUE);

	RemoveEntryList(&ccb->NextCCB);

	ExReleaseResourceLite(&fcb->Resource);
	KeLeaveCriticalRegion();

	ExDeleteResourceLite(&ccb->Resource);

	if (ccb->SearchPattern) {
		ExFreePool(ccb->SearchPattern);
	}

	ExFreeToLookasideListEx(&g_DokanCCBLookasideList, ccb);
	InterlockedIncrement(&fcb->Vcb->CcbFreed);

	return STATUS_SUCCESS;
}

BOOLEAN
DokanCheckCCB(__in PDokanDCB Dcb, __in_opt PDokanCCB Ccb) {
	ASSERT(Dcb != NULL);
	if (GetIdentifierType(Dcb) != DCB) {
		PrintIdType(Dcb);
		return FALSE;
	}

	if (Ccb == NULL || Ccb == 0) {
		PrintIdType(Dcb);
		DDbgPrint("   ccb is NULL\n");
		return FALSE;
	}

	if (Ccb->MountId != Dcb->MountId) {
		DDbgPrint("   MountId is different\n");
		return FALSE;
	}

	if (!Dcb->Mounted) {
		DDbgPrint("  Not mounted\n");
		return FALSE;
	}

	return TRUE;
}
