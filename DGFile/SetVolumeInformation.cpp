
#include "Stdafx.h"
#include "SetVolumeInformation.h"

NTSTATUS
DokanDispatchSetVolumeInformation(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp) {
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PDokanVCB vcb;
	PDokanDCB dcb;
	PIO_STACK_LOCATION irpSp;
	PVOID buffer;
	FS_INFORMATION_CLASS FsInformationClass;

	__try

	{
		DDbgPrint("==> DokanSetVolumeInformation\n");

		vcb = (PDokanVCB)DeviceObject->DeviceExtension;
		if (GetIdentifierType(vcb) != VCB) {
			return STATUS_INVALID_PARAMETER;
		}

		dcb = vcb->Dcb;

		if (!dcb->Mounted) {
			status = STATUS_VOLUME_DISMOUNTED;
			__leave;
		}

		irpSp = IoGetCurrentIrpStackLocation(Irp);
		buffer = Irp->AssociatedIrp.SystemBuffer;
		FsInformationClass = irpSp->Parameters.SetVolume.FsInformationClass;

		switch (FsInformationClass) {
		case FileFsLabelInformation:

			DDbgPrint("  FileFsLabelInformation\n");

			if (sizeof(FILE_FS_LABEL_INFORMATION) >
				irpSp->Parameters.SetVolume.Length)
				return STATUS_INVALID_PARAMETER;

			PFILE_FS_LABEL_INFORMATION Info = (PFILE_FS_LABEL_INFORMATION)buffer;
			ExAcquireResourceExclusiveLite(&dcb->Resource, TRUE);
			if (dcb->VolumeLabel != NULL)
				ExFreePool(dcb->VolumeLabel);
			dcb->VolumeLabel =
				(LPWSTR)ExAllocatePool(Info->VolumeLabelLength + sizeof(WCHAR));
			RtlCopyMemory(dcb->VolumeLabel, Info->VolumeLabel,
				Info->VolumeLabelLength);
			dcb->VolumeLabel[Info->VolumeLabelLength / sizeof(WCHAR)] = '\0';
			ExReleaseResourceLite(&dcb->Resource);
			DDbgPrint(" volume label changed to %ws\n", dcb->VolumeLabel);

			status = STATUS_SUCCESS;
			break;
		}

	}
	__finally {

		DokanCompleteIrpRequest(Irp, status, 0);
	}

	DDbgPrint("<== DokanSetVolumeInformation\n");

	return status;
}
