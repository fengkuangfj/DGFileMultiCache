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

/*++


--*/

#pragma once

#define DGFILE_TAG 'LFGD' // DGFL

//
// DEFINES
//

#define DOKAN_DEBUG_DEFAULT 0

extern ULONG g_Debug;
extern LOOKASIDE_LIST_EX g_DokanCCBLookasideList;
extern LOOKASIDE_LIST_EX g_DokanFCBLookasideList;

#define DOKAN_GLOBAL_DEVICE_NAME L"\\Device\\Dokan" DOKAN_MAJOR_API_VERSION
#define DOKAN_GLOBAL_SYMBOLIC_LINK_NAME                                        \
  L"\\DosDevices\\Global\\Dokan" DOKAN_MAJOR_API_VERSION
#define DOKAN_GLOBAL_FS_DISK_DEVICE_NAME                                       \
  L"\\Device\\DokanFs" DOKAN_MAJOR_API_VERSION
#define DOKAN_GLOBAL_FS_CD_DEVICE_NAME                                         \
  L"\\Device\\DokanCdFs" DOKAN_MAJOR_API_VERSION

#define DOKAN_DISK_DEVICE_NAME L"\\Device\\Volume"
#define DOKAN_SYMBOLIC_LINK_NAME L"\\DosDevices\\Global\\Volume"
#define DOKAN_NET_DEVICE_NAME                                                  \
  L"\\Device\\DokanRedirector" DOKAN_MAJOR_API_VERSION
#define DOKAN_NET_SYMBOLIC_LINK_NAME                                           \
  L"\\DosDevices\\Global\\DokanRedirector" DOKAN_MAJOR_API_VERSION

#define VOLUME_LABEL L"DOKAN"
// {D6CC17C5-1734-4085-BCE7-964F1E9F5DE9}
#define DOKAN_BASE_GUID                                                        \
  {                                                                            \
    0xd6cc17c5, 0x1734, 0x4085, {                                              \
      0xbc, 0xe7, 0x96, 0x4f, 0x1e, 0x9f, 0x5d, 0xe9                           \
    }                                                                          \
  }

#define TAG (ULONG)'AKOD'

#define DOKAN_MDL_ALLOCATED 0x1

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(size) ExAllocatePoolWithTag(NonPagedPool, size, TAG)

#define DRIVER_CONTEXT_EVENT 2
#define DRIVER_CONTEXT_IRP_ENTRY 3

#define DOKAN_IRP_PENDING_TIMEOUT (1000 * 15)               // in millisecond
#define DOKAN_IRP_PENDING_TIMEOUT_RESET_MAX (1000 * 60 * 5) // in millisecond
#define DOKAN_CHECK_INTERVAL (1000 * 5)                     // in millisecond

#define DOKAN_KEEPALIVE_TIMEOUT (1000 * 15) // in millisecond

#if _WIN32_WINNT > 0x501
#define DDbgPrint(...)                                                         \
  if (g_Debug) {                                                               \
    KdPrintEx(                                                                 \
        (DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "[DokanFS] " __VA_ARGS__));  \
  }
#else
#define DDbgPrint(...)                                                         \
  if (g_Debug) {                                                               \
    DbgPrint("[DokanFS] " __VA_ARGS__);                                        \
  }
#endif

#if _WIN32_WINNT < 0x0501
extern PFN_FSRTLTEARDOWNPERSTREAMCONTEXTS DokanFsRtlTeardownPerStreamContexts;
#endif

extern UNICODE_STRING FcbFileNameNull;
#define DokanPrintFileName(FileObject)                                         \
  DDbgPrint("  FileName: %wZ FCB.FileName: %wZ\n", &FileObject->FileName,      \
            FileObject->FsContext2                                             \
                ? (((PDokanCCB)FileObject->FsContext2)->Fcb                    \
                       ? &((PDokanCCB)FileObject->FsContext2)->Fcb->FileName   \
                       : &FcbFileNameNull)                                     \
                : &FcbFileNameNull)

extern NPAGED_LOOKASIDE_LIST DokanIrpEntryLookasideList;
#define DokanAllocateIrpEntry()                                                \
  ExAllocateFromNPagedLookasideList(&DokanIrpEntryLookasideList)
#define DokanFreeIrpEntry(IrpEntry)                                            \
  ExFreeToNPagedLookasideList(&DokanIrpEntryLookasideList, IrpEntry)



#define GetIdentifierType(Obj) (((PFSD_IDENTIFIER)Obj)->Type)

//
// DATA
//




#define IS_DEVICE_READ_ONLY(DeviceObject)                                      \
  (DeviceObject->Characteristics & FILE_READ_ONLY_DEVICE)


//
//  The following macro is used to retrieve the oplock structure within
//  the Fcb. This structure was moved to the advanced Fcb header
//  in Win8.
//
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define DokanGetFcbOplock(F) &(F)->AdvancedFCBHeader.Oplock
#else
#define DokanGetFcbOplock(F) &(F)->Oplock
#endif



typedef struct _DRIVER_EVENT_CONTEXT {
	LIST_ENTRY ListEntry;
	PKEVENT Completed;
	EVENT_CONTEXT EventContext;
} DRIVER_EVENT_CONTEXT, *PDRIVER_EVENT_CONTEXT;

extern "C" DRIVER_INITIALIZE DriverEntry;




NTSTATUS
QueryDeviceRelations(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

DRIVER_UNLOAD DokanUnload;

DRIVER_CANCEL DokanEventCancelRoutine;

DRIVER_CANCEL DokanIrpCancelRoutine;

VOID DokanOplockComplete(IN PVOID Context, IN PIRP Irp);

VOID DokanPrePostIrp(IN PVOID Context, IN PIRP Irp);

DRIVER_DISPATCH DokanRegisterPendingIrpForEvent;

DRIVER_DISPATCH DokanRegisterPendingIrpForService;

DRIVER_DISPATCH DokanCompleteIrp;

DRIVER_DISPATCH DokanResetPendingIrpTimeout;

DRIVER_DISPATCH DokanGetAccessToken;

NTSTATUS
DokanGetMountPointList(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in PDOKAN_GLOBAL dokanGlobal);

NTSTATUS
DokanDispatchRequest(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanEventRelease(__in PDEVICE_OBJECT DeviceObject);

NTSTATUS
DokanGlobalEventRelease(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp);

NTSTATUS
DokanExceptionFilter(__in PIRP Irp, __in PEXCEPTION_POINTERS ExceptionPointer);

NTSTATUS
DokanExceptionHandler(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in NTSTATUS ExceptionCode);

DRIVER_DISPATCH DokanEventStart;

DRIVER_DISPATCH DokanEventWrite;




NTSTATUS
DokanRegisterPendingIrp(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp,
	__in PEVENT_CONTEXT EventContext, __in ULONG Flags);

VOID DokanEventNotification(__in PIRP_LIST NotifyEvent,
	__in PEVENT_CONTEXT EventContext);



VOID DokanNoOpRelease(__in PVOID Fcb);

BOOLEAN
DokanNoOpAcquire(__in PVOID Fcb, __in BOOLEAN Wait);



VOID DokanInitVpb(__in PVPB Vpb, __in PDEVICE_OBJECT VolumeDevice);
VOID DokanDeleteDeviceObject(__in PDokanDCB Dcb);
NTSTATUS IsMountPointDriveLetter(__in PUNICODE_STRING mountPoint);
VOID DokanDeleteMountPoint(__in PDokanDCB Dcb);
VOID DokanPrintNTStatus(NTSTATUS Status);

NTSTATUS DokanRegisterUncProviderSystem(PDokanDCB dcb);
VOID DokanCompleteIrpRequest(__in PIRP Irp, __in NTSTATUS Status,
	__in ULONG_PTR Info);

VOID DokanNotifyReportChange0(__in PDokanFCB Fcb, __in PUNICODE_STRING FileName,
	__in ULONG FilterMatch, __in ULONG Action);

VOID DokanNotifyReportChange(__in PDokanFCB Fcb, __in ULONG FilterMatch,
	__in ULONG Action);

PDokanFCB DokanAllocateFCB(__in PDokanVCB Vcb);

NTSTATUS
DokanFreeFCB(__in PDokanFCB Fcb);

PDokanCCB DokanAllocateCCB(__in PDokanDCB Dcb, __in PDokanFCB Fcb);

NTSTATUS
DokanFreeCCB(__in PDokanCCB Ccb);

NTSTATUS
DokanStartCheckThread(__in PDokanDCB Dcb);

VOID DokanStopCheckThread(__in PDokanDCB Dcb);

BOOLEAN
DokanCheckCCB(__in PDokanDCB Dcb, __in_opt PDokanCCB Ccb);

VOID DokanInitIrpList(__in PIRP_LIST IrpList);



VOID DokanStopEventNotificationThread(__in PDokanDCB Dcb);

VOID DokanUpdateTimeout(__out PLARGE_INTEGER KickCount, __in ULONG Timeout);

VOID DokanUnmount(__in PDokanDCB Dcb);

PMOUNT_ENTRY
FindMountEntry(__in PDOKAN_GLOBAL dokanGlobal,
	__out PDOKAN_CONTROL DokanControl);

VOID PrintIdType(__in VOID *Id);

NTSTATUS
DokanAllocateMdl(__in PIRP Irp, __in ULONG Length);

VOID DokanFreeMdl(__in PIRP Irp);

PUNICODE_STRING
DokanAllocateUnicodeString(__in PCWSTR String);

ULONG
PointerAlignSize(ULONG sizeInBytes);

VOID DokanCreateMountPoint(__in PDokanDCB Dcb);
NTSTATUS DokanSendVolumeArrivalNotification(PUNICODE_STRING DeviceName);

static UNICODE_STRING sddl = RTL_CONSTANT_STRING(
	L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGWGX;;;WD)(A;;GRGX;;;RC)");
