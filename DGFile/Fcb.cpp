
#include "Stdafx.h"
#include "Fcb.h"

// We must NOT call without VCB lock
PDokanFCB DokanAllocateFCB(__in PDokanVCB Vcb) {
	PDokanFCB fcb = (PDokanFCB)ExAllocateFromLookasideListEx(&g_DokanFCBLookasideList);

	if (fcb == NULL) {
		return NULL;
	}

	ASSERT(Vcb != NULL);

	RtlZeroMemory(fcb, sizeof(DokanFCB));

	fcb->Identifier.Type = FCB;
	fcb->Identifier.Size = sizeof(DokanFCB);

	fcb->Vcb = Vcb;

	ExInitializeResourceLite(&fcb->MainResource);
	ExInitializeResourceLite(&fcb->PagingIoResource);

	ExInitializeFastMutex(&fcb->AdvancedFCBHeaderMutex);

#if _WIN32_WINNT >= 0x0501
	FsRtlSetupAdvancedHeader(&fcb->AdvancedFCBHeader,
		&fcb->AdvancedFCBHeaderMutex);
#else
	if (DokanFsRtlTeardownPerStreamContexts) {
		FsRtlSetupAdvancedHeader(&fcb->AdvancedFCBHeader,
			&fcb->AdvancedFCBHeaderMutex);
	}
#endif

	fcb->AdvancedFCBHeader.ValidDataLength.LowPart = 0xffffffff;
	fcb->AdvancedFCBHeader.ValidDataLength.HighPart = 0x7fffffff;

	fcb->AdvancedFCBHeader.Resource = &fcb->MainResource;
	fcb->AdvancedFCBHeader.PagingIoResource = &fcb->PagingIoResource;

	fcb->AdvancedFCBHeader.AllocationSize.QuadPart = 4096;
	fcb->AdvancedFCBHeader.FileSize.QuadPart = 4096;

	fcb->AdvancedFCBHeader.IsFastIoPossible = FastIoIsNotPossible;
	FsRtlInitializeOplock(DokanGetFcbOplock(fcb));

	ExInitializeResourceLite(&fcb->Resource);

	InitializeListHead(&fcb->NextCCB);
	InsertTailList(&Vcb->NextFCB, &fcb->NextFCB);

	InterlockedIncrement(&Vcb->FcbAllocated);

	return fcb;
}

PDokanFCB DokanGetFCB(__in PDokanVCB Vcb, __in PWCHAR FileName,
	__in ULONG FileNameLength) {
	PLIST_ENTRY thisEntry, nextEntry, listHead;
	PDokanFCB fcb = NULL;
	ULONG pos;

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);

	// search the FCB which is already allocated
	// (being used now)
	listHead = &Vcb->NextFCB;

	for (thisEntry = listHead->Flink; thisEntry != listHead;
		thisEntry = nextEntry) {

		nextEntry = thisEntry->Flink;

		fcb = CONTAINING_RECORD(thisEntry, DokanFCB, NextFCB);
		DDbgPrint("  DokanGetFCB has entry FileName: %wZ FileCount: %lu. Looking "
			"for %ls\n",
			fcb->FileName, fcb->FileCount, FileName);
		if (fcb->FileName.Length == FileNameLength) {
			// FileNameLength in bytes
			for (pos = 0; pos < FileNameLength / sizeof(WCHAR); ++pos) {
				if (fcb->FileName.Buffer[pos] != FileName[pos])
					break;
			}
			// we have the FCB which is already allocated and used
			if (pos == FileNameLength / sizeof(WCHAR))
				break;
		}

		fcb = NULL;
	}

	// we don't have FCB
	if (fcb == NULL) {
		DDbgPrint("  Allocate FCB\n");

		fcb = DokanAllocateFCB(Vcb);

		// no memory?
		if (fcb == NULL) {
			DDbgPrint("    Was not able to get FCB for FileName %ls\n", FileName);
			ExFreePool(FileName);
			ExReleaseResourceLite(&Vcb->Resource);
			KeLeaveCriticalRegion();
			return NULL;
		}

		ASSERT(fcb != NULL);

		fcb->FileName.Buffer = FileName;
		fcb->FileName.Length = (USHORT)FileNameLength;
		fcb->FileName.MaximumLength = (USHORT)FileNameLength;

		// we already have FCB
	}
	else {
		// FileName (argument) is never used and must be freed
		ExFreePool(FileName);
	}

	InterlockedIncrement(&fcb->FileCount);

	ExReleaseResourceLite(&Vcb->Resource);
	KeLeaveCriticalRegion();

	return fcb;
}

NTSTATUS
DokanFreeFCB(__in PDokanFCB Fcb) {
	PDokanVCB vcb;

	ASSERT(Fcb != NULL);

	vcb = Fcb->Vcb;

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&vcb->Resource, TRUE);
	ExAcquireResourceExclusiveLite(&Fcb->Resource, TRUE);

	InterlockedDecrement(&Fcb->FileCount);

	if (Fcb->FileCount == 0) {

		RemoveEntryList(&Fcb->NextFCB);

		DDbgPrint("  Free FCB:%p\n", Fcb);
		ExFreePool(Fcb->FileName.Buffer);

		FsRtlUninitializeOplock(DokanGetFcbOplock(Fcb));

#if _WIN32_WINNT >= 0x0501
		FsRtlTeardownPerStreamContexts(&Fcb->AdvancedFCBHeader);
#else
		if (DokanFsRtlTeardownPerStreamContexts) {
			DokanFsRtlTeardownPerStreamContexts(&Fcb->AdvancedFCBHeader);
		}
#endif
		ExReleaseResourceLite(&Fcb->Resource);

		ExDeleteResourceLite(&Fcb->Resource);
		ExDeleteResourceLite(&Fcb->MainResource);
		ExDeleteResourceLite(&Fcb->PagingIoResource);

		InterlockedIncrement(&vcb->FcbFreed);
		ExFreeToLookasideListEx(&g_DokanFCBLookasideList, Fcb);

	}
	else {
		ExReleaseResourceLite(&Fcb->Resource);
	}

	ExReleaseResourceLite(&vcb->Resource);
	KeLeaveCriticalRegion();

	return STATUS_SUCCESS;
}
