
#pragma once

#define DISPATCH_TAG 'PSID' // DISP

__drv_dispatchType(IRP_MJ_CREATE) __drv_dispatchType(IRP_MJ_CLOSE)
__drv_dispatchType(IRP_MJ_READ) __drv_dispatchType(IRP_MJ_WRITE)
__drv_dispatchType(IRP_MJ_FLUSH_BUFFERS) __drv_dispatchType(
	IRP_MJ_CLEANUP) __drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
	__drv_dispatchType(IRP_MJ_FILE_SYSTEM_CONTROL) __drv_dispatchType(
		IRP_MJ_DIRECTORY_CONTROL)
	__drv_dispatchType(IRP_MJ_QUERY_INFORMATION) __drv_dispatchType(
		IRP_MJ_SET_INFORMATION)
	__drv_dispatchType(IRP_MJ_QUERY_VOLUME_INFORMATION)
	__drv_dispatchType(IRP_MJ_SET_VOLUME_INFORMATION)
	__drv_dispatchType(
		IRP_MJ_SHUTDOWN) __drv_dispatchType(IRP_MJ_PNP)
	__drv_dispatchType(IRP_MJ_LOCK_CONTROL)
	__drv_dispatchType(IRP_MJ_QUERY_SECURITY)
	__drv_dispatchType(
		IRP_MJ_SET_SECURITY)

	NTSTATUS
	DokanBuildRequest(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanDispatchRequest(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
