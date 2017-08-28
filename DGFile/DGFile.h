
#pragma once

#define DGFILE_TAG								'LFGD'	// DGFL
#define IRP_ENTRY_LOOKASIDE_LIST_TAG			'EPRI'	// IRPE
#define CCB_LOOKASIDE_LIST_TAG					'LBCC'	// CCBL
#define FCB_LOOKASIDE_LIST_TAG					'LBCF'	// FCBL

#define DGFILE_DEVICE_NAME						L"\\Device\\DGFile"
#define DGFILE_SYMBOLIC_LINK_NAME				L"\\DosDevices\\Global\\DGFile"
#define DGFILE_DISK_FILE_SYSTEM_DEVICE_NAME		L"\\Device\\DGFileDiskFileSystem"
#define DGFILE_CD_ROM_FILE_SYSTEM_DEVICE_NAME	L"\\Device\\DGFileCdRomFileSystem"
#define	DGFILE_SDDL								L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGWGX;;;WD)(A;;GRGX;;;RC)"

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

typedef struct _DokanDiskControlBlock
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
} DokanDCB, *PDokanDCB;

typedef struct _DokanVolumeControlBlock {

	FSD_IDENTIFIER Identifier;

	FSRTL_ADVANCED_FCB_HEADER VolumeFileHeader;
	SECTION_OBJECT_POINTERS SectionObjectPointers;
	FAST_MUTEX AdvancedFCBHeaderMutex;

	ERESOURCE Resource;
	PDEVICE_OBJECT DeviceObject;
	PDokanDCB Dcb;
	LIST_ENTRY NextFCB;

	// NotifySync is used by notify directory change
	PNOTIFY_SYNC NotifySync;
	LIST_ENTRY DirNotifyList;

	LONG FcbAllocated;
	LONG FcbFreed;
	LONG CcbAllocated;
	LONG CcbFreed;

	BOOLEAN HasEventWait;

} DokanVCB, *PDokanVCB;

typedef struct _DGFILE_FCB
{
	FSD_IDENTIFIER Identifier;

	FSRTL_ADVANCED_FCB_HEADER AdvancedFCBHeader;
	SECTION_OBJECT_POINTERS SectionObjectPointers;

	FAST_MUTEX AdvancedFCBHeaderMutex;

	ERESOURCE MainResource;
	ERESOURCE PagingIoResource;

	PDokanVCB Vcb;
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
} DGFILE_FCB, *PDGFILE_FCB, *LPDGFILE_FCB;

typedef struct _DGFILE_CCB
{
	FSD_IDENTIFIER Identifier;
	ERESOURCE Resource;
	LPDGFILE_FCB Fcb;
	LIST_ENTRY NextCCB;
	ULONG64 Context;
	ULONG64 UserContext;

	PWCHAR SearchPattern;
	ULONG SearchPatternLength;

	ULONG Flags;

	int FileCount;
	ULONG MountId;
} DGFILE_CCB, *PDGFILE_CCB, *LPDGFILE_CCB;

typedef enum _FSD_IDENTIFIER_TYPE
{
	DGL = ':DGL', // Dgfile Global
	DCB = ':DCB', // Disk Control Block
	VCB = ':VCB', // Volume Control Block
	FCB = ':FCB', // File Control Block
	CCB = ':CCB', // Context Control Block
} FSD_IDENTIFIER_TYPE, *PFSD_IDENTIFIER_TYPE, *LPFSD_IDENTIFIER_TYPE;

typedef struct _FSD_IDENTIFIER
{
	FSD_IDENTIFIER_TYPE Type;
	ULONG				Size;
} FSD_IDENTIFIER, *PFSD_IDENTIFIER, *LPFSD_IDENTIFIER;

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

//
// Device driver routine declarations.
//
extern "C" DRIVER_INITIALIZE DriverEntry;

NTSTATUS
DriverEntry(
	__inout	PDRIVER_OBJECT	pDriverObject,
	__in	PUNICODE_STRING	pRegistryPath
);

class CDGFile
{
public:
	static LPDGFILE_GLOBAL			spl_lpDgfileGlobal;
	static PFAST_IO_DISPATCH		spl_pFastIoDispatch;
	static FS_FILTER_CALLBACKS		spl_FsFilterCallbacks;
	static NPAGED_LOOKASIDE_LIST	spl_LookasideListIrpEntry;
	static LOOKASIDE_LIST_EX		spl_LookasideListCcb;
	static LOOKASIDE_LIST_EX		spl_LookasideListFcb;
	static BOOLEAN					spl_bLookasideListIrpEntryInit;
	static BOOLEAN					spl_bLookasideListCcbInit;
	static BOOLEAN					spl_bLookasideListFcbInit;

	static
		VOID
		DriverUnload(
			__in PDRIVER_OBJECT pDriverObject
		);

	static
		FSD_IDENTIFIER_TYPE
		GetFsdIdentifierType(
			__in LPDGFILE_GLOBAL lpDgfileGlobal
		);

	static
		VOID
		InitIrpList(
			__in PIRP_LIST pIrpList
		);

private:
};
