
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

/*

IOCTL_EVENT_START:
DokanStartEventNotificationThread
NotificationThread
# PendingEvent has pending IPRs (IOCTL_EVENT_WAIT)
# NotifyEvent has IO events (ex.IRP_MJ_READ)
# notify NotifyEvent using PendingEvent in this loop
NotificationLoop(&Dcb->PendingEvent,
&Dcb->NotifyEvent);

# PendingService has service events (ex. Unmount notification)
# NotifyService has pending IRPs (IOCTL_SERVICE_WAIT)
NotificationLoop(Dcb->Global->PendingService,
&Dcb->Global->NotifyService);

IOCTL_EVENT_RELEASE:
DokanStopEventNotificationThread

IRP_MJ_READ:
DokanDispatchRead
DokanRegisterPendingIrp
# add IRP_MJ_READ to PendingIrp list
DokanRegisterPendingIrpMain(PendingIrp)
# put MJ_READ event into NotifyEvent
DokanEventNotification(NotifyEvent, EventContext)

IOCTL_EVENT_WAIT:
DokanRegisterPendingIrpForEvent
# add this irp to PendingEvent list
DokanRegisterPendingIrpMain(PendingEvent)

IOCTL_EVENT_INFO:
DokanCompleteIrp
DokanCompleteRead

*/

#include "Stdafx.h"
#include "Notification.h"

VOID DokanFreeEventContext(__in PEVENT_CONTEXT EventContext) {
	PDRIVER_EVENT_CONTEXT driverEventContext =
		CONTAINING_RECORD(EventContext, DRIVER_EVENT_CONTEXT, EventContext);
	ExFreePool(driverEventContext);
}

VOID DokanEventNotification(__in PIRP_LIST NotifyEvent,
	__in PEVENT_CONTEXT EventContext) {
	PDRIVER_EVENT_CONTEXT driverEventContext =
		CONTAINING_RECORD(EventContext, DRIVER_EVENT_CONTEXT, EventContext);

	InitializeListHead(&driverEventContext->ListEntry);

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

	// DDbgPrint("DokanEventNotification\n");

	ExInterlockedInsertTailList(&NotifyEvent->ListHead,
		&driverEventContext->ListEntry,
		&NotifyEvent->ListLock);

	KeSetEvent(&NotifyEvent->NotEmpty, IO_NO_INCREMENT, FALSE);
}

VOID ReleaseNotifyEvent(__in PIRP_LIST NotifyEvent) {
	PDRIVER_EVENT_CONTEXT driverEventContext;
	PLIST_ENTRY listHead;
	KIRQL oldIrql;

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	KeAcquireSpinLock(&NotifyEvent->ListLock, &oldIrql);

	while (!IsListEmpty(&NotifyEvent->ListHead)) {
		listHead = RemoveHeadList(&NotifyEvent->ListHead);
		driverEventContext =
			CONTAINING_RECORD(listHead, DRIVER_EVENT_CONTEXT, ListEntry);
		ExFreePool(driverEventContext);
	}

	KeClearEvent(&NotifyEvent->NotEmpty);
	KeReleaseSpinLock(&NotifyEvent->ListLock, oldIrql);
}

VOID NotificationLoop(__in PIRP_LIST PendingIrp, __in PIRP_LIST NotifyEvent) {
	PDRIVER_EVENT_CONTEXT driverEventContext;
	PLIST_ENTRY listHead;
	PIRP_ENTRY irpEntry;
	LIST_ENTRY completeList;
	KIRQL irpIrql;
	KIRQL notifyIrql;
	PIRP irp;
	ULONG eventLen;
	ULONG bufferLen;
	PVOID buffer;

	DDbgPrint("=> NotificationLoop\n");

	InitializeListHead(&completeList);

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	DDbgPrint("Try acquire SpinLock...\n");
	KeAcquireSpinLock(&PendingIrp->ListLock, &irpIrql);
	DDbgPrint("SpinLock irp Acquired\n");
	KeAcquireSpinLock(&NotifyEvent->ListLock, &notifyIrql);
	DDbgPrint("SpinLock notify Acquired\n");

	while (!IsListEmpty(&PendingIrp->ListHead) &&
		!IsListEmpty(&NotifyEvent->ListHead)) {

		listHead = RemoveHeadList(&NotifyEvent->ListHead);

		driverEventContext =
			CONTAINING_RECORD(listHead, DRIVER_EVENT_CONTEXT, ListEntry);

		listHead = RemoveHeadList(&PendingIrp->ListHead);
		irpEntry = CONTAINING_RECORD(listHead, IRP_ENTRY, ListEntry);

		eventLen = driverEventContext->EventContext.Length;

		// ensure this eventIrp is not cancelled
		irp = irpEntry->Irp;

		if (irp == NULL) {
			// this IRP has already been canceled
			DDbgPrint("Irp canceled\n");
			ASSERT(irpEntry->CancelRoutineFreeMemory == FALSE);
			DokanFreeIrpEntry(irpEntry);
			// push back
			InsertTailList(&NotifyEvent->ListHead, &driverEventContext->ListEntry);
			continue;
		}

		if (IoSetCancelRoutine(irp, NULL) == NULL) {
			DDbgPrint("IoSetCancelRoutine return NULL\n");
			// Cancel routine will run as soon as we release the lock
			InitializeListHead(&irpEntry->ListEntry);
			irpEntry->CancelRoutineFreeMemory = TRUE;
			// push back
			InsertTailList(&NotifyEvent->ListHead, &driverEventContext->ListEntry);
			continue;
		}

		// available size that is used for event notification
		bufferLen = irpEntry->IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
		// buffer that is used to inform Event
		buffer = irp->AssociatedIrp.SystemBuffer;

		// buffer is not specified or short of length
		if (bufferLen == 0 || buffer == NULL || bufferLen < eventLen) {
			DDbgPrint("EventNotice : STATUS_INSUFFICIENT_RESOURCES\n");
			DDbgPrint("  bufferLen: %d, eventLen: %d\n", bufferLen, eventLen);
			// push back
			InsertTailList(&NotifyEvent->ListHead, &driverEventContext->ListEntry);
			// marks as STATUS_INSUFFICIENT_RESOURCES
			irpEntry->SerialNumber = 0;
		}
		else {
			// let's copy EVENT_CONTEXT
			RtlCopyMemory(buffer, &driverEventContext->EventContext, eventLen);
			// save event length
			irpEntry->SerialNumber = eventLen;

			if (driverEventContext->Completed) {
				KeSetEvent(driverEventContext->Completed, IO_NO_INCREMENT, FALSE);
			}
			ExFreePool(driverEventContext);
		}
		InsertTailList(&completeList, &irpEntry->ListEntry);
	}

	DDbgPrint("Clear Events...\n");
	KeClearEvent(&NotifyEvent->NotEmpty);
	DDbgPrint("Notify event cleared\n");
	KeClearEvent(&PendingIrp->NotEmpty);
	DDbgPrint("Pending event cleared\n");

	DDbgPrint("Release SpinLock...\n");
	KeReleaseSpinLock(&NotifyEvent->ListLock, notifyIrql);
	DDbgPrint("SpinLock notify Released\n");
	KeReleaseSpinLock(&PendingIrp->ListLock, irpIrql);
	DDbgPrint("SpinLock irp Released\n");

	while (!IsListEmpty(&completeList)) {
		listHead = RemoveHeadList(&completeList);
		irpEntry = CONTAINING_RECORD(listHead, IRP_ENTRY, ListEntry);
		irp = irpEntry->Irp;
		if (irpEntry->SerialNumber == 0) {
			irp->IoStatus.Information = 0;
			irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		}
		else {
			irp->IoStatus.Information = irpEntry->SerialNumber;
			irp->IoStatus.Status = STATUS_SUCCESS;
		}
		DokanFreeIrpEntry(irpEntry);
		DokanCompleteIrpRequest(irp, irp->IoStatus.Status,
			irp->IoStatus.Information);
	}

	DDbgPrint("<= NotificationLoop\n");
}

VOID NotificationThread(__in PDokanDCB Dcb) {
	PKEVENT events[5];
	PKWAIT_BLOCK waitBlock;
	NTSTATUS status;

	DDbgPrint("==> NotificationThread\n");

	waitBlock = (PKWAIT_BLOCK)ExAllocatePool(sizeof(KWAIT_BLOCK) * 5);
	if (waitBlock == NULL) {
		DDbgPrint("  Can't allocate WAIT_BLOCK\n");
		return;
	}
	events[0] = &Dcb->ReleaseEvent;
	events[1] = &Dcb->NotifyEvent.NotEmpty;
	events[2] = &Dcb->PendingEvent.NotEmpty;
	events[3] = &Dcb->Global->PendingService.NotEmpty;
	events[4] = &Dcb->Global->NotifyService.NotEmpty;

	do {
		status = KeWaitForMultipleObjects(5, (PVOID *)events, WaitAny, Executive, KernelMode,
			FALSE, NULL, waitBlock);

		if (status != STATUS_WAIT_0) {
			if (status == STATUS_WAIT_1 || status == STATUS_WAIT_2) {
				NotificationLoop(&Dcb->PendingEvent, &Dcb->NotifyEvent);
			}
			else {
				NotificationLoop(&Dcb->Global->PendingService,
					&Dcb->Global->NotifyService);
			}
		}
	} while (status != STATUS_WAIT_0);

	ExFreePool(waitBlock);
	DDbgPrint("<== NotificationThread\n");
}

NTSTATUS
DokanStartEventNotificationThread(__in PDokanDCB Dcb) {
	NTSTATUS status;
	HANDLE thread;

	DDbgPrint("==> DokanStartEventNotificationThread\n");

	KeResetEvent(&Dcb->ReleaseEvent);

	status = PsCreateSystemThread(&thread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
		(PKSTART_ROUTINE)NotificationThread, Dcb);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	ObReferenceObjectByHandle(thread, THREAD_ALL_ACCESS, NULL, KernelMode,
		(PVOID *)&Dcb->EventNotificationThread, NULL);

	ZwClose(thread);

	DDbgPrint("<== DokanStartEventNotificationThread\n");

	return STATUS_SUCCESS;
}

VOID DokanStopEventNotificationThread(__in PDokanDCB Dcb) {
	DDbgPrint("==> DokanStopEventNotificationThread\n");

	if (KeSetEvent(&Dcb->ReleaseEvent, 0, FALSE) > 0 &&
		Dcb->EventNotificationThread) {
		DDbgPrint("Waiting for Notify thread to terminate.\n");
		ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
		KeWaitForSingleObject(Dcb->EventNotificationThread, Executive, KernelMode,
			FALSE, NULL);
		DDbgPrint("Notify thread successfully terminated.\n");
		ObDereferenceObject(Dcb->EventNotificationThread);
		Dcb->EventNotificationThread = NULL;
	}
	DDbgPrint("<== DokanStopEventNotificationThread\n");
}
