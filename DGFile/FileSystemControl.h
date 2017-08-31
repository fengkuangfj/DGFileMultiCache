
#pragma once

#define FILE_SYSTEM_CONTROL_TAG 'CSIF' // FISC

NTSTATUS
DokanDispatchFileSystemControl(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
