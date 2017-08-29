
#pragma once

typedef enum _FSD_IDENTIFIER_TYPE
{
	FSD_IDENTIFIER_TYPE_CCB = ':CCB', // Context Control Block
	FSD_IDENTIFIER_TYPE_DCB = ':DCB', // Disk Control Block
	FSD_IDENTIFIER_TYPE_DGL = ':DGL', // Dgfile Global
	FSD_IDENTIFIER_TYPE_FCB = ':FCB', // File Control Block
	FSD_IDENTIFIER_TYPE_VCB = ':VCB', // Volume Control Block
} FSD_IDENTIFIER_TYPE, *PFSD_IDENTIFIER_TYPE, *LPFSD_IDENTIFIER_TYPE;

typedef struct _FSD_IDENTIFIER
{
	FSD_IDENTIFIER_TYPE Type;
	ULONG				Size;
} FSD_IDENTIFIER, *PFSD_IDENTIFIER, *LPFSD_IDENTIFIER;

typedef struct _IRP_LIST
{
	LIST_ENTRY ListHead;
	KEVENT NotEmpty;
	KSPIN_LOCK ListLock;
} IRP_LIST, *PIRP_LIST, *LPIRP_LIST;

typedef struct _IRP_ENTRY
{
	LIST_ENTRY			ListEntry;
	ULONG				SerialNumber;
	PIRP				Irp;
	PIO_STACK_LOCATION	IrpSp;
	PFILE_OBJECT		FileObject;
	BOOLEAN				CancelRoutineFreeMemory;
	ULONG				Flags;
	LARGE_INTEGER		TickCount;
	PIRP_LIST			IrpList;
} IRP_ENTRY, *PIRP_ENTRY, *LPIRP_ENTRY;

typedef struct _DGFILE_GLOBAL
{
	FSD_IDENTIFIER Identifier;

	ERESOURCE Resource;
	PDEVICE_OBJECT DeviceObject;
	PDEVICE_OBJECT FsDiskDeviceObject;
	PDEVICE_OBJECT FsCdDeviceObject;
	ULONG MountId;
	// the list of waiting IRP for mount service
	IRP_LIST PendingService;
	IRP_LIST NotifyService;
	LIST_ENTRY MountPointList;
} DGFILE_GLOBAL, *PDGFILE_GLOBAL, *LPDGFILE_GLOBAL;

typedef struct _DCB
{
	FSD_IDENTIFIER Identifier;

	ERESOURCE Resource;

	LPDGFILE_GLOBAL Global;
	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT DeviceObject;

	PVOID Vcb;

	// the list of waiting Event
	IRP_LIST PendingIrp;
	IRP_LIST PendingEvent;
	IRP_LIST NotifyEvent;

	PUNICODE_STRING DiskDeviceName;
	PUNICODE_STRING SymbolicLinkName;
	PUNICODE_STRING MountPoint;
	PUNICODE_STRING UNCName;
	LPWSTR VolumeLabel;

	DEVICE_TYPE DeviceType;
	DEVICE_TYPE VolumeDeviceType;
	ULONG DeviceCharacteristics;
	HANDLE MupHandle;
	UNICODE_STRING MountedDeviceInterfaceName;
	UNICODE_STRING DiskDeviceInterfaceName;

	// When timeout is occuerd, KillEvent is triggered.
	KEVENT KillEvent;

	KEVENT ReleaseEvent;

	// the thread to deal with timeout
	PKTHREAD TimeoutThread;
	PKTHREAD EventNotificationThread;

	// When UseAltStream is 1, use Alternate stream
	USHORT UseAltStream;
	USHORT Mounted;
	USHORT UseMountManager;
	USHORT MountGlobally;
	USHORT FileLockInUserMode;

	// to make a unique id for pending IRP
	ULONG SerialNumber;

	ULONG MountId;

	LARGE_INTEGER TickCount;

	CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
	CACHE_MANAGER_CALLBACKS CacheManagerNoOpCallbacks;

	ULONG IrpTimeout;
} DCB, *PDCB, *LPDCB;

typedef struct _VCB
{
	FSD_IDENTIFIER Identifier;

	FSRTL_ADVANCED_FCB_HEADER VolumeFileHeader;
	SECTION_OBJECT_POINTERS SectionObjectPointers;
	FAST_MUTEX AdvancedFCBHeaderMutex;

	ERESOURCE Resource;
	PDEVICE_OBJECT DeviceObject;
	PDCB Dcb;
	LIST_ENTRY NextFCB;

	// NotifySync is used by notify directory change
	PNOTIFY_SYNC NotifySync;
	LIST_ENTRY DirNotifyList;

	LONG FcbAllocated;
	LONG FcbFreed;
	LONG CcbAllocated;
	LONG CcbFreed;

	BOOLEAN HasEventWait;

} VCB, *PVCB, *LPVCB;

typedef struct _FCB
{
	FSD_IDENTIFIER Identifier;

	FSRTL_ADVANCED_FCB_HEADER AdvancedFCBHeader;
	SECTION_OBJECT_POINTERS SectionObjectPointers;

	FAST_MUTEX AdvancedFCBHeaderMutex;

	ERESOURCE MainResource;
	ERESOURCE PagingIoResource;

	PVCB Vcb;
	LIST_ENTRY NextFCB;
	ERESOURCE Resource;
	LIST_ENTRY NextCCB;

	LONG FileCount;

	ULONG Flags;
	SHARE_ACCESS ShareAccess;

	UNICODE_STRING FileName;

	FILE_LOCK FileLock;

#if (NTDDI_VERSION < NTDDI_WIN8)
	//
	//  The following field is used by the oplock module
	//  to maintain current oplock information.
	//
	OPLOCK Oplock;
#endif

	// uint32 ReferenceCount;
	// uint32 OpenHandleCount;
} FCB, *PFCB, *LPFCB;

typedef struct _CCB
{
	FSD_IDENTIFIER Identifier;

	ERESOURCE Resource;
	LPFCB Fcb;
	LIST_ENTRY NextCCB;
	ULONG64 Context;
	ULONG64 UserContext;

	PWCHAR SearchPattern;
	ULONG SearchPatternLength;

	ULONG Flags;

	int FileCount;
	ULONG MountId;
} CCB, *PCCB, *LPCCB;
