
#pragma once

#define NOTIFICATION_TAG 'ITON' // NOTI

VOID DokanEventNotification(__in PIRP_LIST NotifyEvent,
	__in PEVENT_CONTEXT EventContext);

VOID ReleaseNotifyEvent(__in PIRP_LIST NotifyEvent);

VOID NotificationLoop(__in PIRP_LIST PendingIrp, __in PIRP_LIST NotifyEvent);

VOID NotificationThread(__in PDokanDCB Dcb);

NTSTATUS
DokanStartEventNotificationThread(__in PDokanDCB Dcb);

VOID DokanStopEventNotificationThread(__in PDokanDCB Dcb);

VOID DokanEventNotification(__in PIRP_LIST NotifyEvent,
	__in PEVENT_CONTEXT EventContext);
