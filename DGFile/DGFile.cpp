
#include "Stdafx.h"
#include "DGFile.h"

#pragma warning(disable : 4127)
#pragma warning(disable : 4509)

LPDGFILE_GLOBAL			CDGFile::spl_lpDgfileGlobal = NULL;
NPAGED_LOOKASIDE_LIST	CDGFile::spl_LookasideListIrpEntry = { 0 };
FS_FILTER_CALLBACKS		CDGFile::spl_FsFilterCallbacks = { 0 };
LOOKASIDE_LIST_EX		CDGFile::spl_LookasideListCcb = { 0 };
LOOKASIDE_LIST_EX		CDGFile::spl_LookasideListFcb = { 0 };
BOOLEAN					CDGFile::spl_bLookasideListIrpEntryInit = FALSE;
BOOLEAN					CDGFile::spl_bLookasideListCcbInit = FALSE;
BOOLEAN					CDGFile::spl_bLookasideListFcbInit = FALSE;

NTSTATUS
DriverEntry(
	__inout	PDRIVER_OBJECT	pDriverObject,
	__in	PUNICODE_STRING	pRegistryPath
)
{
	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;

	PDEVICE_OBJECT	pDeviceObject = NULL;
	PDEVICE_OBJECT	pDiskFileSystemDeviceObject = NULL;
	PDEVICE_OBJECT	pCdRomFileSystemDeviceObject = NULL;

	CKrnlStr		strDeviceName;
	CKrnlStr		strSymbolicLinkName;
	CKrnlStr		strDiskFileSystemDeviceName;
	CKrnlStr		strCdRomFileSystemDeviceName;
	CKrnlStr		strSddl;

	CLog			Log;

	UNREFERENCED_PARAMETER(pRegistryPath);

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		if (!Log.Init())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Log.Init failed");
			__leave;
		}

		// LookasideList
		ExInitializeNPagedLookasideList(
			&CDGFile::spl_LookasideListIrpEntry,
			NULL,
			NULL,
			0,
			sizeof(IRP_ENTRY),
			IRP_ENTRY_LOOKASIDE_LIST_TAG,
			0
		);
		CDGFile::spl_bLookasideListIrpEntryInit = TRUE;

		ntStatus = ExInitializeLookasideListEx(
			&CDGFile::spl_LookasideListCcb,
			NULL,
			NULL,
			NonPagedPool,
			0,
			sizeof(DGFILE_CCB),
			CCB_LOOKASIDE_LIST_TAG,
			0
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ExInitializeLookasideListEx failed. (%x)", ntStatus);
			__leave;
		}
		CDGFile::spl_bLookasideListCcbInit = TRUE;

		ntStatus = ExInitializeLookasideListEx(
			&CDGFile::spl_LookasideListFcb,
			NULL,
			NULL,
			NonPagedPool,
			0,
			sizeof(DGFILE_FCB),
			FCB_LOOKASIDE_LIST_TAG,
			0
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ExInitializeLookasideListEx failed. (%x)", ntStatus);
			__leave;
		}
		CDGFile::spl_bLookasideListFcbInit = TRUE;

		strSddl.Set(DGFILE_SDDL, (USHORT)wcslen(DGFILE_SDDL));

		// Device
		strDeviceName.Set(DGFILE_DEVICE_NAME, (USHORT)wcslen(DGFILE_DEVICE_NAME));

		ntStatus = IoCreateDeviceSecure(
			pDriverObject,
			sizeof(DGFILE_GLOBAL),
			strDeviceName.Get(),
			FILE_DEVICE_UNKNOWN,
			0,
			FALSE,
			strSddl.Get(),
			NULL,
			&pDeviceObject
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IoCreateDeviceSecure failed. (%x)", ntStatus);
			__leave;
		}

		// ObReferenceObject(pDeviceObject);

		strSymbolicLinkName.Set(DGFILE_SYMBOLIC_LINK_NAME, (USHORT)wcslen(DGFILE_SYMBOLIC_LINK_NAME));

		ntStatus = IoCreateSymbolicLink(strSymbolicLinkName.Get(), strDeviceName.Get());
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IoCreateSymbolicLink failed. (%x)", ntStatus);
			__leave;
		}

		CDGFile::spl_lpDgfileGlobal = (LPDGFILE_GLOBAL)pDeviceObject->DeviceExtension;

		CDGFile::spl_lpDgfileGlobal->DeviceObject = pDeviceObject;

		CDGFile::InitIrpList(&CDGFile::spl_lpDgfileGlobal->PendingService);
		CDGFile::InitIrpList(&CDGFile::spl_lpDgfileGlobal->NotifyService);
		InitializeListHead(&CDGFile::spl_lpDgfileGlobal->MountPointList);

		CDGFile::spl_lpDgfileGlobal->Identifier.Type = DGL;
		CDGFile::spl_lpDgfileGlobal->Identifier.Size = sizeof(DGFILE_GLOBAL);

		// DiskFileSystemDevice
		strDiskFileSystemDeviceName.Set(DGFILE_DISK_FILE_SYSTEM_DEVICE_NAME, (USHORT)wcslen(DGFILE_DISK_FILE_SYSTEM_DEVICE_NAME));

		ntStatus = IoCreateDeviceSecure(
			pDriverObject,
			0,
			strDiskFileSystemDeviceName.Get(),
			FILE_DEVICE_DISK_FILE_SYSTEM,
			0,
			FALSE,
			strSddl.Get(),
			NULL,
			&pDiskFileSystemDeviceObject
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IoCreateDeviceSecure failed. (%x)", ntStatus);
			__leave;
		}

		// ObReferenceObject(pDiskFileSystemDeviceObject);

		CDGFile::spl_lpDgfileGlobal->FsDiskDeviceObject = pDiskFileSystemDeviceObject;

		pDiskFileSystemDeviceObject->Flags |= DO_DIRECT_IO;
		pDiskFileSystemDeviceObject->Flags |= DO_LOW_PRIORITY_FILESYSTEM;

		pDiskFileSystemDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

		IoRegisterFileSystem(pDiskFileSystemDeviceObject);

		// CdRomFileSystemDevice
		strCdRomFileSystemDeviceName.Set(DGFILE_CD_ROM_FILE_SYSTEM_DEVICE_NAME, (USHORT)wcslen(DGFILE_CD_ROM_FILE_SYSTEM_DEVICE_NAME));

		ntStatus = IoCreateDeviceSecure(
			pDriverObject,
			0,
			strCdRomFileSystemDeviceName.Get(),
			FILE_DEVICE_CD_ROM_FILE_SYSTEM,
			0,
			FALSE,
			strSddl.Get(),
			NULL,
			&pCdRomFileSystemDeviceObject
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IoCreateDeviceSecure failed. (%x)", ntStatus);
			__leave;
		}

		// ObReferenceObject(pCdRomFileSystemDeviceObject);

		CDGFile::spl_lpDgfileGlobal->FsCdDeviceObject = pCdRomFileSystemDeviceObject;

		pCdRomFileSystemDeviceObject->Flags |= DO_DIRECT_IO;
		pCdRomFileSystemDeviceObject->Flags |= DO_LOW_PRIORITY_FILESYSTEM;

		pCdRomFileSystemDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

		IoRegisterFileSystem(pCdRomFileSystemDeviceObject);

		// Callbacks
		pDriverObject->DriverUnload = CDGFile::DriverUnload;

		// 		DriverObject->MajorFunction[IRP_MJ_CREATE] = DokanBuildRequest;
		// 		DriverObject->MajorFunction[IRP_MJ_CLOSE] = DokanBuildRequest;
		// 		DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DokanBuildRequest;
		// 
		// 		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DokanBuildRequest;
		pDriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = CDGFile::Dispatch;
		// 		DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = DokanBuildRequest;
		// 
		// 		DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = DokanBuildRequest;
		// 		DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = DokanBuildRequest;
		// 
		// 		DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = DokanBuildRequest;
		// 		DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = DokanBuildRequest;
		// 
		// 		DriverObject->MajorFunction[IRP_MJ_READ] = DokanBuildRequest;
		// 		DriverObject->MajorFunction[IRP_MJ_WRITE] = DokanBuildRequest;
		// 		DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = DokanBuildRequest;
		// 
		// 		DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DokanBuildRequest;
		// 		DriverObject->MajorFunction[IRP_MJ_PNP] = DokanBuildRequest;
		// 
		// 		DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = DokanBuildRequest;
		// 
		// 		DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = DokanBuildRequest;
		// 		DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] = DokanBuildRequest;

		pDriverObject->FastIoDispatch = new(DGFILE_TAG)FAST_IO_DISPATCH;

		pDriverObject->FastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
		// 		CDGFile::spl_pFastIoDispatch->FastIoCheckIfPossible = DokanFastIoCheckIfPossible;
		// 		CDGFile::spl_pFastIoDispatch->FastIoRead = FsRtlCopyRead;
		// 		CDGFile::spl_pFastIoDispatch->FastIoWrite = FsRtlCopyWrite;
		// 		CDGFile::spl_pFastIoDispatch->AcquireFileForNtCreateSection = DokanAcquireForCreateSection;
		// 		CDGFile::spl_pFastIoDispatch->ReleaseFileForNtCreateSection = DokanReleaseForCreateSection;
		// 		CDGFile::spl_pFastIoDispatch->MdlRead = FsRtlMdlReadDev;
		// 		CDGFile::spl_pFastIoDispatch->MdlReadComplete = FsRtlMdlReadCompleteDev;
		// 		CDGFile::spl_pFastIoDispatch->PrepareMdlWrite = FsRtlPrepareMdlWriteDev;
		// 		CDGFile::spl_pFastIoDispatch->MdlWriteComplete = FsRtlMdlWriteCompleteDev;

		CDGFile::spl_FsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
		// filterCallbacks.PreAcquireForSectionSynchronization = DokanFilterCallbackAcquireForCreateSection;

		ntStatus = FsRtlRegisterFileSystemFilterCallbacks(pDriverObject, &CDGFile::spl_FsFilterCallbacks);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FsRtlRegisterFileSystemFilterCallbacks failed. (%x)", ntStatus);
			__leave;
		}
	}
	__finally
	{
		if (!NT_SUCCESS(ntStatus))
		{
			if (CDGFile::spl_lpDgfileGlobal)
			{
				if (CDGFile::spl_lpDgfileGlobal->FsDiskDeviceObject)
				{
					IoDeleteDevice(CDGFile::spl_lpDgfileGlobal->FsDiskDeviceObject);
					CDGFile::spl_lpDgfileGlobal->FsDiskDeviceObject = NULL;
				}

				if (CDGFile::spl_lpDgfileGlobal->FsCdDeviceObject)
				{
					IoDeleteDevice(CDGFile::spl_lpDgfileGlobal->FsCdDeviceObject);
					CDGFile::spl_lpDgfileGlobal->FsCdDeviceObject = NULL;
				}

				if (CDGFile::spl_lpDgfileGlobal->DeviceObject)
				{
					IoDeleteDevice(CDGFile::spl_lpDgfileGlobal->DeviceObject);
					CDGFile::spl_lpDgfileGlobal->DeviceObject = NULL;
				}
			}

			if (CDGFile::spl_bLookasideListIrpEntryInit)
				ExDeleteNPagedLookasideList(&CDGFile::spl_LookasideListIrpEntry);

			if (CDGFile::spl_bLookasideListCcbInit)
				ExDeleteLookasideListEx(&CDGFile::spl_LookasideListCcb);

			if (CDGFile::spl_bLookasideListFcbInit)
				ExDeleteLookasideListEx(&CDGFile::spl_LookasideListFcb);

			delete pDriverObject->FastIoDispatch;
			pDriverObject->FastIoDispatch = NULL;

			Log.Unload();
		}
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return ntStatus;
}

VOID
CDGFile::DriverUnload(
	__in PDRIVER_OBJECT pDriverObject
)
{
	PDEVICE_OBJECT	pDeviceObject = NULL;
	LPDGFILE_GLOBAL	lpDgfileGlobal = NULL;
	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;

	CKrnlStr		strGlobalSymbolicLinkName;

	CLog			Log;

	UNREFERENCED_PARAMETER(pDriverObject);

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		pDeviceObject = pDriverObject->DeviceObject;
		if (pDeviceObject)
		{
			lpDgfileGlobal = (LPDGFILE_GLOBAL)pDeviceObject->DeviceExtension;
			if (DGL == CDGFile::GetFsdIdentifierType(lpDgfileGlobal))
			{
				strGlobalSymbolicLinkName.Set(DGFILE_SYMBOLIC_LINK_NAME, (USHORT)wcslen(DGFILE_SYMBOLIC_LINK_NAME));
				ntStatus = IoDeleteSymbolicLink(strGlobalSymbolicLinkName.Get());
				if (!NT_SUCCESS(ntStatus))
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IoDeleteSymbolicLink failed. (%x)", ntStatus);

				if (lpDgfileGlobal->FsDiskDeviceObject)
				{
					IoUnregisterFileSystem(lpDgfileGlobal->FsDiskDeviceObject);
					IoDeleteDevice(lpDgfileGlobal->FsDiskDeviceObject);
					lpDgfileGlobal->FsDiskDeviceObject = NULL;
				}

				if (lpDgfileGlobal->FsCdDeviceObject)
				{
					IoUnregisterFileSystem(lpDgfileGlobal->FsCdDeviceObject);
					IoDeleteDevice(lpDgfileGlobal->FsCdDeviceObject);
					lpDgfileGlobal->FsCdDeviceObject = NULL;
				}

				IoDeleteDevice(pDeviceObject);
				pDeviceObject = NULL;
			}
		}

		if (CDGFile::spl_bLookasideListIrpEntryInit)
			ExDeleteNPagedLookasideList(&CDGFile::spl_LookasideListIrpEntry);

		if (CDGFile::spl_bLookasideListCcbInit)
			ExDeleteLookasideListEx(&CDGFile::spl_LookasideListCcb);

		if (CDGFile::spl_bLookasideListFcbInit)
			ExDeleteLookasideListEx(&CDGFile::spl_LookasideListFcb);

		delete pDriverObject->FastIoDispatch;
		pDriverObject->FastIoDispatch = NULL;

		Log.Unload();
	}
	__finally
	{
		;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return;
}

FSD_IDENTIFIER_TYPE
CDGFile::GetFsdIdentifierType(
	__in LPFSD_IDENTIFIER lpFsdIdentifier
)
{

	FSD_IDENTIFIER_TYPE FsdIdentifierType = (FSD_IDENTIFIER_TYPE)0;


	__try
	{
		if (!lpFsdIdentifier)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error.");
			__leave;
		}

		FsdIdentifierType = lpFsdIdentifier->Type;
	}
	__finally
	{
		;
	}

	return FsdIdentifierType;
}

VOID
CDGFile::InitIrpList(
	__in PIRP_LIST pIrpList
)
{
	InitializeListHead(&pIrpList->ListHead);
	KeInitializeSpinLock(&pIrpList->ListLock);
	KeInitializeEvent(&pIrpList->NotEmpty, NotificationEvent, FALSE);
}

BOOLEAN
CDGFile::InitList(
	__in CKrnlStr *		pstrKey,
	__in CKrnlStr *		pstrValue,
	__in CKrnlStr **	pstrList,
	__in PULONG			pulCount
)
{
	BOOLEAN							bRet = FALSE;

	OBJECT_ATTRIBUTES				ObjectAttributes = { 0 };
	NTSTATUS						ntStatus = STATUS_UNSUCCESSFUL;
	HANDLE							hKey = NULL;
	PKEY_VALUE_PARTIAL_INFORMATION	pKeyValuePartialInfo = NULL;
	ULONG							ulRet = 0;
	PWCHAR							pPosition = NULL;
	ULONG							ulIndex = 0;

	CKrnlStr						Data;
	CKrnlStr						DataTemp;


	__try
	{
		if (!pstrKey || !pstrValue || !pulCount)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input raguments error");
			__leave;
		}

		InitializeObjectAttributes(
			&ObjectAttributes,
			pstrKey->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
		);

		ntStatus = ZwOpenKey(
			&hKey,
			KEY_ALL_ACCESS,
			&ObjectAttributes
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ZwOpenKey failed. (%x) (%wZ)",
				ntStatus, pstrKey->Get());

			__leave;
		}

		pKeyValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)new(DGFILE_TAG)CHAR[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 1024];

		ntStatus = ZwQueryValueKey(
			hKey,
			pstrValue->Get(),
			KeyValuePartialInformation,
			pKeyValuePartialInfo,
			sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 1024,
			&ulRet
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ZwQueryValueKey failed. (%x) (%wZ) (%wZ)",
				ntStatus, pstrKey->Get(), pstrValue->Get());

			__leave;
		}

		if (!Data.Set((PWCHAR)pKeyValuePartialInfo->Data, (USHORT)(pKeyValuePartialInfo->DataLength / sizeof(WCHAR))))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Data.Set failed");
			__leave;
		}

		if (1 <= Data.GetLenCh() &&
			L'\0' == *(Data.GetString() + Data.GetLenCh() - 1))
		{
			if (!Data.Shorten(Data.GetLenCh() - 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Data.Shorten failed. (%wZ)", Data.Get());
				__leave;
			}
		}

		if (!DataTemp.Set(&Data))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"DataTemp.Set failed. (%wZ)", Data.Get());
			__leave;
		}

		do
		{
			pPosition = DataTemp.SearchCharacter(L'|', DataTemp.GetString() + DataTemp.GetLenCh() - 1, DataTemp.GetString());
			if (!pPosition)
			{
				(*pulCount)++;
				break;
			}

			if (!DataTemp.Shorten((USHORT)(pPosition - DataTemp.GetString())))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"DataTemp.Shorten failed. (%wZ)",
					DataTemp.Get());

				__leave;
			}

			(*pulCount)++;
		} while (TRUE);

		if (!*pulCount)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"*pulControlSysNameCount error");
			__leave;
		}

		*pstrList = new(DGFILE_TAG)CKrnlStr[*pulCount];

		do
		{
			pPosition = Data.SearchCharacter(L'|', Data.GetString() + Data.GetLenCh() - 1, Data.GetString());
			if (!pPosition)
			{
				if (!(*pstrList)[ulIndex].Set(&Data))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_pControlSysNameList[%d].Set failed. (%wZ)",
						ulIndex, Data.Get());

					__leave;
				}

				break;
			}

			if (!(*pstrList)[ulIndex].Set(pPosition + 1, (USHORT)(Data.GetLenCh() - (pPosition - Data.GetString() + 1))))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_pControlSysNameList[%d].Set failed",
					ulIndex);

				__leave;
			}

			if (!Data.Shorten((USHORT)(pPosition - Data.GetString())))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Data.Shorten failed. (%wZ)",
					Data.Get());

				__leave;
			}

			ulIndex++;

			if (ulIndex >= *pulCount)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_ulControlSysNameCount too small. %d >= %d",
					ulIndex, *pulCount);

				__leave;
			}
		} while (TRUE);

		bRet = TRUE;
	}
	__finally
	{
		delete pKeyValuePartialInfo;
		pKeyValuePartialInfo = NULL;

		if (hKey)
		{
			ZwClose(hKey);
			hKey = NULL;
		}

		if (!bRet && pstrList)
		{
			delete[] * pstrList;
			*pstrList = NULL;
		}
	}

	return bRet;
}

NTSTATUS
CDGFile::Dispatch(
	__in PDEVICE_OBJECT pDeviceObject,
	__in PIRP			pIrp
)
{
	NTSTATUS			ntStatus = STATUS_UNSUCCESSFUL;

	BOOLEAN				bFsRtlEnterFileSystem = FALSE;
	PIRP				pTopLevelIrpOrg = NULL;
	PIO_STACK_LOCATION	pIoStackLocation = NULL;

	__try
	{
		if (APC_LEVEL >= KeGetCurrentIrql())
		{
			FsRtlEnterFileSystem();
			bFsRtlEnterFileSystem = TRUE;
		}

		pTopLevelIrpOrg = IoGetTopLevelIrp();
		if (!pTopLevelIrpOrg)
			IoSetTopLevelIrp(pIrp);

		pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);
		if (!pIoStackLocation)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IoGetCurrentIrpStackLocation failed.");
			__leave;
		}
		if (IsDeviceReadOnly(pDeviceObject))
		{
			if ((IRP_MJ_WRITE == pIoStackLocation->MajorFunction) ||
				(IRP_MJ_SET_INFORMATION == pIoStackLocation->MajorFunction) ||
				(IRP_MJ_SET_EA == pIoStackLocation->MajorFunction) ||
				(IRP_MJ_FLUSH_BUFFERS == pIoStackLocation->MajorFunction) ||
				(IRP_MJ_SET_SECURITY == pIoStackLocation->MajorFunction) ||
				(IRP_MJ_SET_VOLUME_INFORMATION == pIoStackLocation->MajorFunction) ||
				(IRP_MJ_FILE_SYSTEM_CONTROL == pIoStackLocation->MajorFunction &&
					IRP_MN_USER_FS_REQUEST == pIoStackLocation->MinorFunction &&
					FSCTL_MARK_VOLUME_DIRTY == pIoStackLocation->Parameters.FileSystemControl.FsControlCode))
			{
				ntStatus = STATUS_MEDIA_WRITE_PROTECTED;
				__leave;
			}
		}

		switch (pIoStackLocation->MajorFunction)
		{
		case IRP_MJ_CREATE:
		{
			ntStatus = Create(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_CLOSE:
		{
			ntStatus = Close(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_READ:
		{
			ntStatus = Read(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_WRITE:
		{
			ntStatus = Write(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_FLUSH_BUFFERS:
		{
			ntStatus = FlushBuffers(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_QUERY_INFORMATION:
		{
			ntStatus = QueryInformation(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_SET_INFORMATION:
		{
			ntStatus = SetInformation(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_QUERY_VOLUME_INFORMATION:
		{
			ntStatus = QueryVolumeInformation(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_SET_VOLUME_INFORMATION:
		{
			ntStatus = SetVolumeInformation(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_DIRECTORY_CONTROL:
		{
			ntStatus = DirectoryControl(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_FILE_SYSTEM_CONTROL:
		{
			ntStatus = FileSystemControl(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_DEVICE_CONTROL:
		{
			ntStatus = DeviceControl(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_LOCK_CONTROL:
		{
			ntStatus = LockControl(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_CLEANUP:
		{
			ntStatus = Cleanup(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_SHUTDOWN:
		{
			ntStatus = Shutdown(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_QUERY_SECURITY:
		{
			ntStatus = QuerySecurity(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_SET_SECURITY:
		{
			ntStatus = SetSecurity(pDeviceObject, pIrp);
			break;
		}
		case IRP_MJ_PNP:
		{
			ntStatus = Pnp(pDeviceObject, pIrp);
			break;
		}
		default:
		{
			DokanCompleteIrpRequest(pIrp, STATUS_DRIVER_INTERNAL_ERROR, 0);

			ntStatus = STATUS_DRIVER_INTERNAL_ERROR;
			break;
		}
		}
	}
	__finally
	{
		if (!pTopLevelIrpOrg)
			IoSetTopLevelIrp(pTopLevelIrpOrg);

		if (bFsRtlEnterFileSystem && APC_LEVEL >= KeGetCurrentIrql())
			FsRtlExitFileSystem();
	}

	return ntStatus;
}


BOOLEAN
CDGFile::IsDeviceReadOnly(
	__in PDEVICE_OBJECT pDeviceObject
)
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!pDeviceObject)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error.");
			__leave;
		}

		bRet = FlagOn(pDeviceObject->Characteristics, FILE_READ_ONLY_DEVICE);
	}
	__finally
	{
		;
	}
	
	return bRet;
}
