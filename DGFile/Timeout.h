
#pragma once

#define TIMEOUT_TAG 'EMIT' // TIME

VOID DokanCheckKeepAlive(__in PDokanDCB Dcb);

NTSTATUS
ReleaseTimeoutPendingIrp(__in PDokanDCB Dcb);

NTSTATUS
DokanResetPendingIrpTimeout(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

VOID DokanTimeoutThread(PDokanDCB Dcb);

NTSTATUS
DokanStartCheckThread(__in PDokanDCB Dcb);

VOID DokanStopCheckThread(__in PDokanDCB Dcb);

NTSTATUS
DokanInformServiceAboutUnmount(__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp);

VOID DokanUpdateTimeout(__out PLARGE_INTEGER TickCount, __in ULONG Timeout);
