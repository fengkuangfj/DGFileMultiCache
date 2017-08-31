
#include "Stdafx.h"
#include "Vpb.h"

VOID DokanInitVpb(__in PVPB Vpb, __in PDEVICE_OBJECT VolumeDevice) {
	if (Vpb != NULL) {
		Vpb->DeviceObject = VolumeDevice;
		Vpb->VolumeLabelLength = (USHORT)wcslen(VOLUME_LABEL) * sizeof(WCHAR);
		RtlStringCchCopyW(Vpb->VolumeLabel,
			sizeof(Vpb->VolumeLabel) / sizeof(WCHAR), VOLUME_LABEL);
		Vpb->SerialNumber = 0x19831116;
	}
}
