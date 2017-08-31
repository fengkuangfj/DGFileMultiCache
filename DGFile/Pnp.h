
#pragma once

#define PNP_TAG 'TPNP' // PNPT

NTSTATUS
DokanDispatchPnp(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);
