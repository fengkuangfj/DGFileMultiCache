
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
			__in LPFSD_IDENTIFIER lpFsdIdentifier
		);

	static
		BOOLEAN
		IsDeviceReadOnly(
		__in PDEVICE_OBJECT pDeviceObject
		);

	static
		VOID
		InitIrpList(
			__in PIRP_LIST pIrpList
		);

	static
		BOOLEAN
		InitList(
			__in CKrnlStr *		pstrKey,
			__in CKrnlStr *		pstrValue,
			__in CKrnlStr **	pstrList,
			__in PULONG			pulCount
		);

	static
		NTSTATUS
		Dispatch(
			__in PDEVICE_OBJECT pDeviceObject,
			__in PIRP			pIrp
		);

	static
		NTSTATUS
		Create(
			__in PDEVICE_OBJECT pDeviceObject,
			__in PIRP			pIrp
		);

private:
};
