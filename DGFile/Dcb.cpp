
#include "Stdafx.h"
#include "Dcb.h"

VOID FreeDcbNames(__in PDokanDCB Dcb) {
	if (Dcb->MountPoint != NULL) {
		FreeUnicodeString(Dcb->MountPoint);
		Dcb->MountPoint = NULL;
	}
	if (Dcb->SymbolicLinkName != NULL) {
		FreeUnicodeString(Dcb->SymbolicLinkName);
		Dcb->SymbolicLinkName = NULL;
	}
	if (Dcb->DiskDeviceName != NULL) {
		FreeUnicodeString(Dcb->DiskDeviceName);
		Dcb->DiskDeviceName = NULL;
	}
	if (Dcb->UNCName != NULL) {
		FreeUnicodeString(Dcb->UNCName);
		Dcb->UNCName = NULL;
	}
}
