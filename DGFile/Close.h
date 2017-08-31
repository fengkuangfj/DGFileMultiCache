
#pragma once

#define CLOSE_TAG 'SOLC' // CLOS

NTSTATUS
DokanDispatchClose(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
