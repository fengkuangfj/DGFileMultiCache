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

#pragma once

#ifndef NTDDI_WIN8
#define NTDDI_WIN8                          0x06020000
#endif

#ifndef NTDDI_WINBLUE
#define NTDDI_WINBLUE                       0x06030000
#endif

#ifndef NTDDI_WINTHRESHOLD
#define NTDDI_WINTHRESHOLD                  0x0A000000  /* ABRACADABRA_THRESHOLD */
#endif

#ifndef NTDDI_WIN10
#define NTDDI_WIN10                         0x0A000000  /* ABRACADABRA_THRESHOLD */
#endif

#ifndef NTDDI_WIN10_TH2
#define NTDDI_WIN10_TH2                     0x0A000001  /* ABRACADABRA_WIN10_TH2 */
#endif

#ifndef DOKAN_MAJOR_API_VERSION
#define DOKAN_MAJOR_API_VERSION L"1"
#endif

#define DOKAN_DRIVER_VERSION 0x0000190

#define EVENT_CONTEXT_MAX_SIZE (1024 * 32)

#define IOCTL_TEST                                                             \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_DEBUG_MODE                                                   \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_EVENT_WAIT                                                       \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_EVENT_INFO                                                       \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_EVENT_RELEASE                                                    \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_EVENT_START                                                      \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_EVENT_WRITE                                                      \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define IOCTL_KEEPALIVE                                                        \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x809, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_SERVICE_WAIT                                                     \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80A, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_RESET_TIMEOUT                                                    \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80B, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_ACCESS_TOKEN                                                 \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80C, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_EVENT_MOUNTPOINT_LIST                                            \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80D, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DRIVER_FUNC_INSTALL 0x01
#define DRIVER_FUNC_REMOVE 0x02

#define DOKAN_MOUNTED 1
#define DOKAN_USED 2
#define DOKAN_START_FAILED 3

#define DOKAN_DEVICE_MAX 10

#define DOKAN_DEFAULT_SECTOR_SIZE 512
#define DOKAN_DEFAULT_ALLOCATION_UNIT_SIZE 512
#define DOKAN_DEFAULT_DISK_SIZE 1024 * 1024 * 1024

// used in CCB->Flags and FCB->Flags
#define DOKAN_FILE_DIRECTORY 1
#define DOKAN_FILE_DELETED 2
#define DOKAN_FILE_OPENED 4
#define DOKAN_DIR_MATCH_ALL 8
#define DOKAN_DELETE_ON_CLOSE 16
#define DOKAN_PAGING_IO 32
#define DOKAN_SYNCHRONOUS_IO 64
#define DOKAN_WRITE_TO_END_OF_FILE 128
#define DOKAN_NOCACHE 256

// used in DOKAN_START->DeviceType
#define DOKAN_DISK_FILE_SYSTEM 0
#define DOKAN_NETWORK_FILE_SYSTEM 1

/*
* This structure is used for copying UNICODE_STRING from the kernel mode driver
* into the user mode driver.
* https://msdn.microsoft.com/en-us/library/windows/hardware/ff564879(v=vs.85).aspx
*/
typedef struct _DOKAN_UNICODE_STRING_INTERMEDIATE {
	USHORT Length;
	USHORT MaximumLength;
	WCHAR Buffer[1];
} DOKAN_UNICODE_STRING_INTERMEDIATE, *PDOKAN_UNICODE_STRING_INTERMEDIATE;

/*
* This structure is used for copying ACCESS_STATE from the kernel mode driver
* into the user mode driver.
* https://msdn.microsoft.com/en-us/library/windows/hardware/ff538840(v=vs.85).aspx
*/
typedef struct _DOKAN_ACCESS_STATE_INTERMEDIATE {
	BOOLEAN SecurityEvaluated;
	BOOLEAN GenerateAudit;
	BOOLEAN GenerateOnClose;
	BOOLEAN AuditPrivileges;
	ULONG Flags;
	ACCESS_MASK RemainingDesiredAccess;
	ACCESS_MASK PreviouslyGrantedAccess;
	ACCESS_MASK OriginalDesiredAccess;

	// Offset from the beginning of this structure to a SECURITY_DESCRIPTOR
	// if 0 that means there is no security descriptor
	ULONG SecurityDescriptorOffset;

	// Offset from the beginning of this structure to a
	// DOKAN_UNICODE_STRING_INTERMEDIATE
	ULONG UnicodeStringObjectNameOffset;

	// Offset from the beginning of this structure to a
	// DOKAN_UNICODE_STRING_INTERMEDIATE
	ULONG UnicodeStringObjectTypeOffset;
} DOKAN_ACCESS_STATE_INTERMEDIATE, *PDOKAN_ACCESS_STATE_INTERMEDIATE;

typedef struct _DOKAN_ACCESS_STATE {
	BOOLEAN SecurityEvaluated;
	BOOLEAN GenerateAudit;
	BOOLEAN GenerateOnClose;
	BOOLEAN AuditPrivileges;
	ULONG Flags;
	ACCESS_MASK RemainingDesiredAccess;
	ACCESS_MASK PreviouslyGrantedAccess;
	ACCESS_MASK OriginalDesiredAccess;
	PSECURITY_DESCRIPTOR SecurityDescriptor;
	UNICODE_STRING ObjectName;
	UNICODE_STRING ObjectType;
} DOKAN_ACCESS_STATE, *PDOKAN_ACCESS_STATE;

/*
* This structure is used for copying IO_SECURITY_CONTEXT from the kernel mode
* driver into the user mode driver.
* https://msdn.microsoft.com/en-us/library/windows/hardware/ff550613(v=vs.85).aspx
*/
typedef struct _DOKAN_IO_SECURITY_CONTEXT_INTERMEDIATE {
	DOKAN_ACCESS_STATE_INTERMEDIATE AccessState;
	ACCESS_MASK DesiredAccess;
} DOKAN_IO_SECURITY_CONTEXT_INTERMEDIATE,
*PDOKAN_IO_SECURITY_CONTEXT_INTERMEDIATE;

typedef struct _DOKAN_IO_SECURITY_CONTEXT {
	DOKAN_ACCESS_STATE AccessState;
	ACCESS_MASK DesiredAccess;
} DOKAN_IO_SECURITY_CONTEXT, *PDOKAN_IO_SECURITY_CONTEXT;

typedef struct _CREATE_CONTEXT {
	DOKAN_IO_SECURITY_CONTEXT_INTERMEDIATE SecurityContext;
	ULONG FileAttributes;
	ULONG CreateOptions;
	ULONG ShareAccess;
	ULONG FileNameLength;

	// Offset from the beginning of this structure to the string
	ULONG FileNameOffset;
} CREATE_CONTEXT, *PCREATE_CONTEXT;

typedef struct _CLEANUP_CONTEXT {
	ULONG FileNameLength;
	WCHAR FileName[1];

} CLEANUP_CONTEXT, *PCLEANUP_CONTEXT;

typedef struct _CLOSE_CONTEXT {
	ULONG FileNameLength;
	WCHAR FileName[1];

} CLOSE_CONTEXT, *PCLOSE_CONTEXT;

typedef struct _DIRECTORY_CONTEXT {
	ULONG FileInformationClass;
	ULONG FileIndex;
	ULONG BufferLength;
	ULONG DirectoryNameLength;
	ULONG SearchPatternLength;
	ULONG SearchPatternOffset;
	WCHAR DirectoryName[1];
	WCHAR SearchPatternBase[1];

} DIRECTORY_CONTEXT, *PDIRECTORY_CONTEXT;

typedef struct _READ_CONTEXT {
	LARGE_INTEGER ByteOffset;
	ULONG BufferLength;
	ULONG FileNameLength;
	WCHAR FileName[1];
} READ_CONTEXT, *PREAD_CONTEXT;

typedef struct _WRITE_CONTEXT {
	LARGE_INTEGER ByteOffset;
	ULONG BufferLength;
	ULONG BufferOffset;
	ULONG RequestLength;
	ULONG FileNameLength;
	WCHAR FileName[2];
	// "2" means to keep last null of contents to write
} WRITE_CONTEXT, *PWRITE_CONTEXT;

typedef struct _FILEINFO_CONTEXT {
	ULONG FileInformationClass;
	ULONG BufferLength;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILEINFO_CONTEXT, *PFILEINFO_CONTEXT;

typedef struct _SETFILE_CONTEXT {
	ULONG FileInformationClass;
	ULONG BufferLength;
	ULONG BufferOffset;
	ULONG FileNameLength;
	WCHAR FileName[1];
} SETFILE_CONTEXT, *PSETFILE_CONTEXT;

typedef struct _VOLUME_CONTEXT {
	ULONG FsInformationClass;
	ULONG BufferLength;
} VOLUME_CONTEXT, *PVOLUME_CONTEXT;

typedef struct _LOCK_CONTEXT {
	LARGE_INTEGER ByteOffset;
	LARGE_INTEGER Length;
	ULONG Key;
	ULONG FileNameLength;
	WCHAR FileName[1];
} LOCK_CONTEXT, *PLOCK_CONTEXT;

typedef struct _FLUSH_CONTEXT {
	ULONG FileNameLength;
	WCHAR FileName[1];
} FLUSH_CONTEXT, *PFLUSH_CONTEXT;

typedef struct _UNMOUNT_CONTEXT {
	WCHAR DeviceName[64];
	ULONG Option;
} UNMOUNT_CONTEXT, *PUNMOUNT_CONTEXT;

typedef struct _SECURITY_CONTEXT {
	SECURITY_INFORMATION SecurityInformation;
	ULONG BufferLength;
	ULONG FileNameLength;
	WCHAR FileName[1];
} SECURITY_CONTEXT, *PSECURITY_CONTEXT;

typedef struct _SET_SECURITY_CONTEXT {
	SECURITY_INFORMATION SecurityInformation;
	ULONG BufferLength;
	ULONG BufferOffset;
	ULONG FileNameLength;
	WCHAR FileName[1];
} SET_SECURITY_CONTEXT, *PSET_SECURITY_CONTEXT;

typedef struct _EVENT_CONTEXT {
	ULONG Length;
	ULONG MountId;
	ULONG SerialNumber;
	ULONG ProcessId;
	UCHAR MajorFunction;
	UCHAR MinorFunction;
	ULONG Flags;
	ULONG FileFlags;
	ULONG64 Context;
	union {
		DIRECTORY_CONTEXT Directory;
		READ_CONTEXT Read;
		WRITE_CONTEXT Write;
		FILEINFO_CONTEXT File;
		CREATE_CONTEXT Create;
		CLOSE_CONTEXT Close;
		SETFILE_CONTEXT SetFile;
		CLEANUP_CONTEXT Cleanup;
		LOCK_CONTEXT Lock;
		VOLUME_CONTEXT Volume;
		FLUSH_CONTEXT Flush;
		UNMOUNT_CONTEXT Unmount;
		SECURITY_CONTEXT Security;
		SET_SECURITY_CONTEXT SetSecurity;
	} Operation;
} EVENT_CONTEXT, *PEVENT_CONTEXT;

#define WRITE_MAX_SIZE                                                         \
  (EVENT_CONTEXT_MAX_SIZE - sizeof(EVENT_CONTEXT) - 256 * sizeof(WCHAR))

typedef struct _EVENT_INFORMATION {
	ULONG SerialNumber;
	NTSTATUS Status;
	ULONG Flags;
	union {
		struct {
			ULONG Index;
		} Directory;
		struct {
			ULONG Flags;
			ULONG Information;
		} Create;
		struct {
			LARGE_INTEGER CurrentByteOffset;
		} Read;
		struct {
			LARGE_INTEGER CurrentByteOffset;
		} Write;
		struct {
			UCHAR DeleteOnClose;
		} Delete;
		struct {
			ULONG Timeout;
		} ResetTimeout;
		struct {
			HANDLE Handle;
		} AccessToken;
	} Operation;
	ULONG64 Context;
	ULONG BufferLength;
	UCHAR Buffer[8];

} EVENT_INFORMATION, *PEVENT_INFORMATION;

#define DOKAN_EVENT_ALTERNATIVE_STREAM_ON 1
#define DOKAN_EVENT_WRITE_PROTECT 2
#define DOKAN_EVENT_REMOVABLE 4
#define DOKAN_EVENT_MOUNT_MANAGER 8
#define DOKAN_EVENT_CURRENT_SESSION 16
#define DOKAN_EVENT_FILELOCK_USER_MODE 32

typedef struct _EVENT_DRIVER_INFO {
	ULONG DriverVersion;
	ULONG Status;
	ULONG DeviceNumber;
	ULONG MountId;
	WCHAR DeviceName[64];
} EVENT_DRIVER_INFO, *PEVENT_DRIVER_INFO;

typedef struct _EVENT_START {
	ULONG UserVersion;
	ULONG DeviceType;
	ULONG Flags;
	WCHAR MountPoint[260];
	WCHAR UNCName[64];
	ULONG IrpTimeout;
} EVENT_START, *PEVENT_START;

typedef struct _DOKAN_RENAME_INFORMATION {
	BOOLEAN ReplaceIfExists;
	ULONG FileNameLength;
	WCHAR FileName[1];
} DOKAN_RENAME_INFORMATION, *PDOKAN_RENAME_INFORMATION;

typedef struct _DOKAN_LINK_INFORMATION {
	BOOLEAN ReplaceIfExists;
	ULONG FileNameLength;
	WCHAR FileName[1];
} DOKAN_LINK_INFORMATION, *PDOKAN_LINK_INFORMATION;


//
// FSD_IDENTIFIER_TYPE
//
// Identifiers used to mark the structures
//
typedef enum _FSD_IDENTIFIER_TYPE {
	DGL = ':DGL', // Dokan Global
	DCB = ':DCB', // Disk Control Block
	VCB = ':VCB', // Volume Control Block
	FCB = ':FCB', // File Control Block
	CCB = ':CCB', // Context Control Block
} FSD_IDENTIFIER_TYPE;

//
// FSD_IDENTIFIER
//
// Header put in the beginning of every structure
//
typedef struct _FSD_IDENTIFIER {
	FSD_IDENTIFIER_TYPE Type;
	ULONG Size;
} FSD_IDENTIFIER, *PFSD_IDENTIFIER;

typedef struct _IRP_LIST {
	LIST_ENTRY ListHead;
	KEVENT NotEmpty;
	KSPIN_LOCK ListLock;
} IRP_LIST, *PIRP_LIST;


typedef struct _DOKAN_GLOBAL {
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
} DOKAN_GLOBAL, *PDOKAN_GLOBAL;


// make sure Identifier is the top of struct
typedef struct _DokanDiskControlBlock {

	FSD_IDENTIFIER Identifier;

	ERESOURCE Resource;

	PDOKAN_GLOBAL Global;
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


typedef struct _DokanFileControlBlock {
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
} DokanFCB, *PDokanFCB;


typedef struct _DokanContextControlBlock {
	FSD_IDENTIFIER Identifier;
	ERESOURCE Resource;
	PDokanFCB Fcb;
	LIST_ENTRY NextCCB;
	ULONG64 Context;
	ULONG64 UserContext;

	PWCHAR SearchPattern;
	ULONG SearchPatternLength;

	ULONG Flags;

	int FileCount;
	ULONG MountId;
} DokanCCB, *PDokanCCB;


typedef struct _DOKAN_CONTROL {
	ULONG Type;            // File System Type
	WCHAR MountPoint[260]; // Mount Point
	WCHAR UNCName[64];
	WCHAR DeviceName[64];        // Disk Device Name
	PDEVICE_OBJECT DeviceObject; // Volume Device Object
} DOKAN_CONTROL, *PDOKAN_CONTROL;

typedef struct _MOUNT_ENTRY {
	LIST_ENTRY ListEntry;
	DOKAN_CONTROL MountControl;
} MOUNT_ENTRY, *PMOUNT_ENTRY;


// IRP list which has pending status
// this structure is also used to store event notification IRP
typedef struct _IRP_ENTRY {
	LIST_ENTRY ListEntry;
	ULONG SerialNumber;
	PIRP Irp;
	PIO_STACK_LOCATION IrpSp;
	PFILE_OBJECT FileObject;
	BOOLEAN CancelRoutineFreeMemory;
	ULONG Flags;
	LARGE_INTEGER TickCount;
	PIRP_LIST IrpList;
} IRP_ENTRY, *PIRP_ENTRY;