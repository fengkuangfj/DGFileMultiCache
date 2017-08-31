
#pragma once

#define VOLUME_TAG 'ULOV' // VOLU

NTSTATUS
DokanSendVolumeArrivalNotification(PUNICODE_STRING DeviceName);

NTSTATUS
DokanSendVolumeDeletePoints(__in PUNICODE_STRING MountPoint,
	__in PUNICODE_STRING DeviceName);

NTSTATUS
DokanSendVolumeCreatePoint(__in PUNICODE_STRING DeviceName,
	__in PUNICODE_STRING MountPoint);
