
#pragma once

#define SHUTDOWN_TAG 'TUHS' // SHUT

NTSTATUS
DokanDispatchShutdown(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
