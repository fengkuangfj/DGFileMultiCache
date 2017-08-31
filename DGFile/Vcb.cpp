
#include "Stdafx.h"
#include "Vcb.h"

VOID SetFileObjectForVCB(__in PFILE_OBJECT FileObject, __in PDokanVCB Vcb) {
	FileObject->SectionObjectPointer = &Vcb->SectionObjectPointers;
	FileObject->FsContext = &Vcb->VolumeFileHeader;
}
