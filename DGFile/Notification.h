
#pragma once

#define NOTIFICATION_TAG 'ITON' // NOTI



VOID SetCommonEventContext(__in PDokanDCB Dcb, __in PEVENT_CONTEXT EventContext,
	__in PIRP Irp, __in_opt PDokanCCB Ccb);

PEVENT_CONTEXT
AllocateEventContextRaw(__in ULONG EventContextLength);

PEVENT_CONTEXT
AllocateEventContext(__in PDokanDCB Dcb, __in PIRP Irp,
	__in ULONG EventContextLength, __in_opt PDokanCCB Ccb);

VOID DokanFreeEventContext(__in PEVENT_CONTEXT EventContext);

VOID DokanEventNotification(__in PIRP_LIST NotifyEvent,
	__in PEVENT_CONTEXT EventContext);

VOID ReleasePendingIrp(__in PIRP_LIST PendingIrp);

VOID ReleaseNotifyEvent(__in PIRP_LIST NotifyEvent);

VOID NotificationLoop(__in PIRP_LIST PendingIrp, __in PIRP_LIST NotifyEvent);

VOID NotificationThread(__in PDokanDCB Dcb);

NTSTATUS
DokanStartEventNotificationThread(__in PDokanDCB Dcb);

VOID DokanStopEventNotificationThread(__in PDokanDCB Dcb);

