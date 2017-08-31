/*
Dokan : user-mode file system library for Windows

Copyright (C) 2015 - 2016 Adrien J. <liryna.stark@gmail.com> and Maxime C. <maxime@islog.com>
Copyright (C) 2007 - 2011 Hiroki Asakawa <info@dokan-dev.net>

http://dokan-dev.github.io

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Stdafx.h"
#include "FileSystemControl.h"

NTSTATUS
DokanDispatchFileSystemControl(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp) {
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PIO_STACK_LOCATION irpSp;

	__try {
		DDbgPrint("==> DokanFileSystemControl\n");
		DDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));

		irpSp = IoGetCurrentIrpStackLocation(Irp);

		switch (irpSp->MinorFunction) {
		case IRP_MN_KERNEL_CALL:
			DDbgPrint("	 IRP_MN_KERNEL_CALL\n");
			break;

		case IRP_MN_LOAD_FILE_SYSTEM:
			DDbgPrint("	 IRP_MN_LOAD_FILE_SYSTEM\n");
			break;

		case IRP_MN_MOUNT_VOLUME: {
			DDbgPrint("	 IRP_MN_MOUNT_VOLUME\n");
			status = DokanMountVolume(DeviceObject, Irp);
		} break;

		case IRP_MN_USER_FS_REQUEST:
			DDbgPrint("	 IRP_MN_USER_FS_REQUEST\n");
			status = DokanUserFsRequest(DeviceObject, &Irp);
			break;

		case IRP_MN_VERIFY_VOLUME:
			DDbgPrint("	 IRP_MN_VERIFY_VOLUME\n");
			break;

		default:
			DDbgPrint("  unknown %d\n", irpSp->MinorFunction);
			status = STATUS_INVALID_PARAMETER;
			break;
		}

	}
	__finally {

		DokanCompleteIrpRequest(Irp, status, 0);

		DDbgPrint("<== DokanFileSystemControl\n");
	}

	return status;
}
