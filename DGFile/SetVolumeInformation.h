
#pragma once

#define SET_VOLUME_INFORMATION_TAG 'IVES' // SEVI

NTSTATUS
DokanDispatchSetVolumeInformation(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp);
