
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：Log.cpp

摘  要：

作  者：岳翔

--*/

#include "stdafx.h"
#include "Log.h"

#pragma warning(disable : 4127)
#pragma warning(disable : 4509)

LIST_ENTRY		CLog::ms_ListHead = { 0 };
ERESOURCE		CLog::ms_Lock = { 0 };
KSPIN_LOCK		CLog::ms_SpLock = 0;
KEVENT			CLog::ms_WaitEvent = { 0 };
CKrnlStr*		CLog::ms_pLogFile = NULL;
CKrnlStr*		CLog::ms_pLogDir = NULL;
HANDLE			CLog::ms_hLogFile = NULL;
PFILE_OBJECT	CLog::ms_pLogFileObj = NULL;
LARGE_INTEGER	CLog::ms_liByteOffset = { 0 };
PFLT_INSTANCE	CLog::ms_pFltInstance = NULL;
ULONG			CLog::ms_ulSectorSize = 0;
PETHREAD		CLog::ms_pEThread = NULL;
BOOLEAN			CLog::ms_bCanInsertLog = FALSE;

CLog::CLog()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

CLog::~CLog()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

VOID
CLog::ThreadStart(
__in PVOID StartContext
)
{
	CLog			Log;

	LARGE_INTEGER	WaitTime = { 0 };
	ULONG			ulCount = 0;
	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;
	BOOLEAN			bCanWriteLog = FALSE;

	CKrnlStr		LogInfo;

	UNREFERENCED_PARAMETER(StartContext);

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"begin");

	__try
	{
		Log.GetLock();

		WaitTime.QuadPart = -100 * 1000;

		do
		{
			Log.FreeLock();
			ntStatus = KeWaitForSingleObject(
				&ms_WaitEvent,
				Executive,
				KernelMode,
				FALSE,
				&WaitTime
				);
			Log.GetLock();
			switch (ntStatus)
			{
			case STATUS_SUCCESS:
			case STATUS_TIMEOUT:
			{
				Log.FreeLock();
				bCanWriteLog = Log.LogFileReady();
				Log.GetLock();
				if (bCanWriteLog)
				{
					do
					{
						if (!Log.Pop(&LogInfo))
						{
							if (STATUS_SUCCESS == ntStatus)
								__leave;

							break;
						}

						Log.FreeLock();
						if (!Log.Write(&LogInfo))
						{
							Log.GetLock();
							KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"Log.Write failed. Log(%wZ)",
								LogInfo.Get());

							__leave;
						}
						Log.GetLock();

						ulCount++;
					} while (ulCount < EVERY_TIME_LOG_MAX_COUNT);
				}
				else
				{
					if (STATUS_SUCCESS != ntStatus)
					{
						Log.FreeLock();
						bCanWriteLog = Log.InitLogFile(TRUE);
						Log.GetLock();
					}
				}

				if (!bCanWriteLog && STATUS_SUCCESS == ntStatus)
					__leave;

				break;
			}
			default:
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"KeWaitForSingleObject failed. (%x)",
					ntStatus);

				__leave;
			}
			}

			ulCount = 0;
		} while (TRUE);
	}
	__finally
	{
		Log.FreeLock();

		if (!Log.ReleaseLogFile())
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RleaseLogFile failed");
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"end");

	return;
}

BOOLEAN
CLog::Init()
{
	BOOLEAN			bRet = FALSE;

	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;
	HANDLE			hThead = NULL;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"begin");

	__try
	{
		InitializeListHead(&ms_ListHead);
		KeInitializeSpinLock(&ms_SpLock);
		ExInitializeResourceLite(&ms_Lock);
		KeInitializeEvent(&ms_WaitEvent, NotificationEvent, FALSE);

		ms_pLogFile = new(LOG_TAG)CKrnlStr;
		ms_pLogDir = new(LOG_TAG)CKrnlStr;

		ntStatus = PsCreateSystemThread(
			&hThead,
			GENERIC_ALL,
			NULL,
			NULL,
			NULL,
			ThreadStart,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"PsCreateSystemThread failed. (%x)",
				ntStatus);

			__leave;
		}

		ntStatus = ObReferenceObjectByHandle(
			hThead,
			GENERIC_ALL,
			*PsThreadType,
			KernelMode,
			(PVOID *)&ms_pEThread,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ObReferenceObjectByHandle failed. (%x)",
				ntStatus);

			__leave;
		}

		KeSetBasePriorityThread(ms_pEThread, -4);

		ms_bCanInsertLog = TRUE;

		bRet = TRUE;
	}
	__finally
	{
		// 不要置为NULL
		if (ms_pEThread)
			ObDereferenceObject(ms_pEThread);

		if (hThead)
		{
			ZwClose(hThead);
			hThead = NULL;
		}

		if (!bRet)
		{
			ExDeleteResourceLite(&ms_Lock);
			RtlZeroMemory(&ms_Lock, sizeof(ms_Lock));

			delete ms_pLogFile;
			ms_pLogFile = NULL;

			delete ms_pLogDir;
			ms_pLogDir = NULL;
		}
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"end");

	return bRet;
}

BOOLEAN
CLog::Unload()
{
	BOOLEAN			bRet = FALSE;

	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;
	KPRIORITY		kPriority = 0;
	PLIST_ENTRY		pNode = NULL;
	LPLOG_INFO		lpLogInfo = NULL;

	CKrnlStr		LogInfo;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"begin");

	__try
	{
		GetLock();

		ms_bCanInsertLog = FALSE;

		FreeLock();
		if (KeSetEvent(&ms_WaitEvent, kPriority, FALSE))
		{
			GetLock();
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"already signaled");
			__leave;
		}

		ntStatus = KeWaitForSingleObject(
			ms_pEThread,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		GetLock();
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"KeWaitForSingleObject failed. (%x)",
				ntStatus);

			__leave;
		}

		while (!IsListEmpty(&ms_ListHead))
		{
			pNode = RemoveHeadList(&ms_ListHead);

			lpLogInfo = CONTAINING_RECORD(pNode, LOG_INFO, List);
			if (!lpLogInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"CONTAINING_RECORD failed");
				__leave;
			}

			delete lpLogInfo;
			lpLogInfo = NULL;
		}

		bRet = TRUE;
	}
	__finally
	{
		delete ms_pLogFile;
		ms_pLogFile = NULL;

		delete ms_pLogDir;
		ms_pLogDir = NULL;

		if (ms_pFltInstance)
		{
			FltObjectDereference(ms_pFltInstance);
			ms_pFltInstance = NULL;
		}

		FreeLock();

		ExDeleteResourceLite(&ms_Lock);
		RtlZeroMemory(&ms_Lock, sizeof(ms_Lock));
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"end");

	return bRet;
}

/*
* 函数说明：
*		获得锁
*
* 参数：
*		无
*
* 返回值：
*		无
*
* 备注：
*		无
*/
#pragma warning(push)
#pragma warning(disable: 28167)
VOID
CLog::GetLock()
{
	__try
	{
		// 判断当前锁引用计数是否合法
		if (m_LockRef < 0)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"m_LockRef < 0 (%d)", m_LockRef);
			__leave;
		}

		// 判断需不需要加锁
		if (m_LockRef++)
			__leave;

		// 加锁
		m_Irql = KeGetCurrentIrql();
		if (m_Irql == DISPATCH_LEVEL)
			KeAcquireSpinLock(&ms_SpLock, &m_Irql);
		else if (m_Irql <= APC_LEVEL)
		{
#pragma warning(push)
#pragma warning(disable: 28103)
			KeEnterCriticalRegion();
			ExAcquireResourceExclusiveLite(&ms_Lock, TRUE);
#pragma warning(pop)
		}
		else
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"IRQL > DISPATCH_LEVEL");
	}
	__finally
	{
		;
	}

	return;
}
#pragma warning(pop)

/*
* 函数说明：
*		释放锁
*
* 参数：
*		无
*
* 返回值：
*		无
*
* 备注：
*		无
*
*/
#pragma warning(push)
#pragma warning(disable: 28167)
VOID
CLog::FreeLock()
{
	__try
	{
		// 判断当前锁引用计数是否合法
		if (m_LockRef <= 0)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"m_LockRef <= 0 (%d)", m_LockRef);
			__leave;
		}

		// 判断需不需要解锁
		if (--m_LockRef)
			__leave;

		// 解锁
		if (m_Irql == DISPATCH_LEVEL)
			KeReleaseSpinLock(&ms_SpLock, m_Irql);
		else if (m_Irql <= APC_LEVEL)
		{
#pragma warning(push)
#pragma warning(disable: 28107)
			ExReleaseResourceLite(&ms_Lock);
			KeLeaveCriticalRegion();
#pragma warning(pop)
		}
		else
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"IRQL > DISPATCH_LEVEL");
	}
	__finally
	{
		;
	}

	return;
}
#pragma warning(pop)

BOOLEAN
CLog::Pop(
__out CKrnlStr * pLog
)
{
	BOOLEAN		bRet = FALSE;

	LPLOG_INFO	lpLogInfo = NULL;
	LIST_ENTRY*	pNode = NULL;


	__try
	{
		GetLock();

		if (!pLog)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		pNode = RemoveHeadList(&ms_ListHead);
		if (pNode == &ms_ListHead)
			__leave;

		lpLogInfo = CONTAINING_RECORD(pNode, LOG_INFO, List);
		if (!lpLogInfo)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"CONTAINING_RECORD failed");
			__leave;
		}

		if (!pLog->Set(&lpLogInfo->Log))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"pLog->Set failed. Log(%wZ)",
				lpLogInfo->Log.Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		delete lpLogInfo;
		lpLogInfo = NULL;

		FreeLock();
	}

	return bRet;
}

BOOLEAN
CLog::Write(
__in CKrnlStr * pLog
)
{
	BOOLEAN			bRet = FALSE;

	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;
	ULONG			ulWriteLen = 0;
	PCHAR  			pWriteBuf = NULL;

	CKrnlStr		Log;


	__try
	{
		GetLock();

		if (!pLog)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		if (!IsSameDate(pLog))
		{
			FreeLock();
			if (!InitLogFileName())
			{
				GetLock();
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"InitLogFileName failed");
				__leave;
			}

			if (!InitLogDir(ms_pLogFile))
			{
				GetLock();
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"InitLogDir failed. (%wZ)",
					ms_pLogFile->Get());

				__leave;
			}

			if (!InitLogFile(TRUE))
			{
				GetLock();
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"InitLogFile failed");
				__leave;
			}
			GetLock();
		}

		if (!Log.Set(pLog))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"Log.Append failed. Log(%wZ)",
				pLog->Get());

			__leave;
		}

		if (!Log.Append(L"\r\n", (USHORT)wcslen(L"\r\n")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"Log.Append failed. Log(%wZ)",
				pLog->Get());

			__leave;
		}

		ntStatus = RtlUnicodeToMultiByteSize(&ulWriteLen, Log.GetString(), Log.GetLenB());
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlUnicodeToMultiByteSize failed. Log(%wZ)",
				pLog->Get());

			__leave;
		}

		pWriteBuf = (PCHAR)FltAllocatePoolAlignedWithTag(
			ms_pFltInstance,
			PagedPool,
			ulWriteLen,
			LOG_TAG
			);
		if (!pWriteBuf)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltAllocatePoolAlignedWithTag failed. ulWriteLen(%d)",
				ulWriteLen);

			__leave;
		}

		RtlZeroMemory(pWriteBuf, ulWriteLen);

		ntStatus = RtlUnicodeToMultiByteN(
			pWriteBuf,
			ulWriteLen,
			NULL,
			Log.GetString(),
			Log.GetLenB()
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlUnicodeToMultiByteN failed. Log(%wZ)",
				pLog->Get());

			__leave;
		}

		FreeLock();
		ntStatus = FltWriteFile(
			ms_pFltInstance,
			ms_pLogFileObj,
			&ms_liByteOffset,
			ulWriteLen,
			pWriteBuf,
			0,
			NULL,
			NULL,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			GetLock();

			if (STATUS_TOO_LATE != ntStatus)
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltWriteFile failed. (%x) Log(%wZ)",
				ntStatus, pLog->Get());

			__leave;
		}
		GetLock();

		ms_liByteOffset.QuadPart += ulWriteLen;

		if (ms_liByteOffset.QuadPart > MAX_LOG_FILE_SIZE)
		{
			FreeLock();
			if (!InitLogFile(TRUE))
			{
				GetLock();
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"InitLogFile failed");
				__leave;
			}
			GetLock();
		}

		bRet = TRUE;
	}
	__finally
	{
		if (pWriteBuf)
		{
			FltFreePoolAlignedWithTag(
				ms_pFltInstance,
				pWriteBuf,
				LOG_TAG
				);

			pWriteBuf = NULL;
		}

		FreeLock();
	}

	return bRet;
}

BOOLEAN
CLog::LogFileReady()
{
	BOOLEAN				bRet = FALSE;

	OBJECT_ATTRIBUTES	Oa = { 0 };
	NTSTATUS			ntStatus = STATUS_UNSUCCESSFUL;
	HANDLE				Handle = NULL;
	IO_STATUS_BLOCK		Iosb = { 0 };


	__try
	{
		GetLock();

		if (!ms_ulSectorSize)
			__leave;

		if (!CMinifilter::CheckEnv(MINIFILTER_ENV_TYPE_FLT_FILTER))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"CMinifilter::CheckEnv failed");
			__leave;
		}

		if (!ms_pFltInstance)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"NULL == ms_pFltInstance");
			__leave;
		}

		if (!ms_pLogFile || !ms_pLogFile->GetString())
			__leave;

		FreeLock();
		if (!InitLogDir(ms_pLogFile))
		{
			GetLock();
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"InitLogDir failed. (%wZ)",
				ms_pLogFile->Get());

			__leave;
		}
		GetLock();

		InitializeObjectAttributes(
			&Oa,
			ms_pLogFile->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		FreeLock();
		ntStatus = FltCreateFile(
			CMinifilter::gFilterHandle,
			ms_pFltInstance,
			&Handle,
			FILE_READ_ATTRIBUTES,
			&Oa,
			&Iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			NULL,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
			NULL,
			0,
			IO_IGNORE_SHARE_ACCESS_CHECK
			);
		GetLock();
		if (!NT_SUCCESS(ntStatus))
		{
			if (STATUS_OBJECT_NAME_NOT_FOUND != ntStatus)
			{
				if (STATUS_DELETE_PENDING == ntStatus)
					KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"FltCreateFile failed. (%x) File(%wZ)",
					ntStatus, ms_pLogFile->Get());
				else
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltCreateFile failed. (%x) File(%wZ)",
					ntStatus, ms_pLogFile->Get());
			}

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();

		if (Handle)
		{
			FltClose(Handle);
			Handle = NULL;
		}
	}

	return bRet;
}

BOOLEAN
CLog::InitLogFile(
__in BOOLEAN bReset
)
{
	BOOLEAN						bRet = FALSE;

	OBJECT_ATTRIBUTES			Oa = { 0 };
	NTSTATUS					ntStatus = STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK				Iosb = { 0 };
	FILE_STANDARD_INFORMATION	FileStandardInfo = { 0 };
	ULONG						ulRet = 0;


	__try
	{
		GetLock();

		if (bReset)
		{
			if (ms_pLogFileObj)
			{
				ObDereferenceObject(ms_pLogFileObj);
				ms_pLogFileObj = NULL;
			}

			if (ms_hLogFile)
			{
				FreeLock();
				FltClose(ms_hLogFile);
				GetLock();
				ms_hLogFile = NULL;
			}

			ms_liByteOffset.QuadPart = 0;
		}

		if (!InitLogFileName())
			__leave;

		if (!CMinifilter::CheckEnv(MINIFILTER_ENV_TYPE_FLT_FILTER))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"NULL == CMinifilter::CheckEnv failed");
			__leave;
		}

		if (!ms_pFltInstance)
			__leave;

		if (!ms_pLogFile || !ms_pLogFile->Get())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile not ready");
			__leave;
		}

		InitializeObjectAttributes(
			&Oa,
			ms_pLogFile->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		FreeLock();
		ntStatus = FltCreateFile(
			CMinifilter::gFilterHandle,
			ms_pFltInstance,
			&ms_hLogFile,
			GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
			&Oa,
			&Iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			NULL,
			FILE_OPEN_IF,
			FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED | FILE_SYNCHRONOUS_IO_NONALERT/* | FILE_NO_INTERMEDIATE_BUFFERING*/,
			NULL,
			0,
			IO_IGNORE_SHARE_ACCESS_CHECK
			);
		GetLock();
		if (!NT_SUCCESS(ntStatus))
		{
			if (STATUS_OBJECT_PATH_NOT_FOUND != ntStatus)
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltCreateFile failed (%x) File(%wZ)",
				ntStatus, ms_pLogFile->Get());

			__leave;
		}

		ntStatus = ObReferenceObjectByHandle(
			ms_hLogFile,
			0,
			NULL,
			KernelMode,
			(PVOID *)&ms_pLogFileObj,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ObReferenceObjectByHandle failed (%x) File(%wZ)",
				ntStatus, ms_pLogFile->Get());

			__leave;
		}

		ntStatus = FltQueryInformationFile(
			ms_pFltInstance,
			ms_pLogFileObj,
			&FileStandardInfo,
			sizeof(FILE_STANDARD_INFORMATION),
			FileStandardInformation,
			&ulRet
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltQueryInformationFile failed. (%x) File(%wZ)",
				ntStatus, ms_pLogFile->Get());

			__leave;
		}

		ms_liByteOffset.QuadPart = (ULONG)ROUND_TO_SIZE(FileStandardInfo.EndOfFile.QuadPart, ms_ulSectorSize);

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();

		if (!bRet)
		{
			if (!ReleaseLogFile())
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RleaseLogFile failed");
		}
	}

	return bRet;
}

BOOLEAN
CLog::InitLogFileName()
{
	BOOLEAN			bRet = FALSE;

	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER	snow = { 0 };
	LARGE_INTEGER	now = { 0 };
	TIME_FIELDS		nowFields = { 0 };
	WCHAR			Time[64] = { 0 };
	WCHAR			Dir[64] = { 0 };


	__try
	{
		GetLock();

		if (!ms_pLogFile)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"NULL == ms_pLogFile");
			__leave;
		}

		if (!ms_pLogDir || !ms_pLogDir->GetString())
			__leave;

		if (!ms_pLogFile->Set(ms_pLogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Set failed. Dir(%wZ)",
				ms_pLogDir->Get());

			__leave;
		}

		if (!ms_pLogFile->Append(L"\\DGFile\\", (USHORT)wcslen(L"\\DGFile\\")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Append failed 1. File(%wZ)",
				ms_pLogFile->Get());

			__leave;
		}

		KeQuerySystemTime(&snow);
		ExSystemTimeToLocalTime(&snow, &now);
		RtlTimeToTimeFields(&now, &nowFields);

		ntStatus = RtlStringCchPrintfW(
			Dir,
			64,
			L"%04d-%02d-%02d",
			nowFields.Year,
			nowFields.Month,
			nowFields.Day
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlStringCchPrintfW failed. (%x)",
				ntStatus);

			__leave;
		}

		if (!ms_pLogFile->Append(Dir, (USHORT)wcslen(Dir)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Append failed. File(%wZ) (%lS)",
				ms_pLogFile->Get(), Dir);

			__leave;
		}

		if (!ms_pLogFile->Append(L"\\", (USHORT)wcslen(L"\\")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Append failed. File(%wZ) (%lS)",
				ms_pLogFile->Get(), Dir);

			__leave;
		}

		ntStatus = RtlStringCchPrintfW(
			Time,
			64,
			L"[%04d-%02d-%02d][%02d-%02d-%02d.%03d]",
			nowFields.Year,
			nowFields.Month,
			nowFields.Day,
			nowFields.Hour,
			nowFields.Minute,
			nowFields.Second,
			nowFields.Milliseconds
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlStringCchPrintfW failed. (%x)",
				ntStatus);

			__leave;
		}

		if (!ms_pLogFile->Append(Time, (USHORT)wcslen(Time)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Append failed. File(%wZ) (%lS)",
				ms_pLogFile->Get(), Time);

			__leave;
		}

		if (!ms_pLogFile->Append(L".log", (USHORT)wcslen(L".log")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Append failed 2. File(%wZ)",
				ms_pLogFile->Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

BOOLEAN
CLog::Insert(
__in WCHAR	*	pLog,
__in USHORT		usLenCh
)
{
	BOOLEAN		bRet = FALSE;

	LPLOG_INFO lpLogInfo = NULL;


	__try
	{
		GetLock();

		if (!pLog)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		if (!ms_bCanInsertLog)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"not ready for insert");
			__leave;
		}

		lpLogInfo = new(LOG_TAG)LOG_INFO;

		if (!lpLogInfo->Log.Set(pLog, usLenCh))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"lpLogInfo->Log.Set failed. Log(%lS)",
				pLog);

			__leave;
		}

		InsertTailList(&ms_ListHead, &lpLogInfo->List);

		bRet = TRUE;
	}
	__finally
	{
		if (!bRet)
		{
			delete lpLogInfo;
			lpLogInfo = NULL;
		}

		FreeLock();
	}

	return bRet;
}

BOOLEAN
CLog::ReleaseLogFile()
{
	BOOLEAN bRet = FALSE;


	__try
	{
		GetLock();

		if (ms_pLogFileObj)
		{
			ObDereferenceObject(ms_pLogFileObj);
			ms_pLogFileObj = NULL;
		}

		if (ms_hLogFile)
		{
			FreeLock();
			FltClose(ms_hLogFile);
			GetLock();
			ms_hLogFile = NULL;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

BOOLEAN
CLog::SetLogDir(
__in CKrnlStr * pLogDir
)
{
	BOOLEAN bRet = FALSE;


	__try
	{
		GetLock();

		if (!pLogDir)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		if (!ms_pLogDir)
			__leave;

		if (!ms_pLogDir->Set(pLogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogDir->Set failed. Dir(%wZ)",
				pLogDir->Get());

			__leave;
		}

		if (!InitFltInstance())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"InitFltInstance failed");
			__leave;
		}

		if (!InitSectorSize())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"InitSectorSize failed");

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

BOOLEAN
CLog::InitFltInstance()
{
	BOOLEAN			bRet = FALSE;

	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;
	PFLT_VOLUME		pFltVolume = NULL;

	CKrnlStr		VolumeName;


	__try
	{
		if (ms_pFltInstance)
		{
			FltObjectDereference(ms_pFltInstance);
			ms_pFltInstance = NULL;
		}

		if (!VolumeName.Set(ms_pLogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Set failed. (%wZ)",
				ms_pLogDir->Get());

			__leave;
		}

		if (24 == VolumeName.GetLenCh())
		{
			if (L'\\' == *(VolumeName.GetString() + VolumeName.GetLenCh() - 1))
			{
				// \Device\HarddiskVolume1\ 
				if (!VolumeName.Shorten(23))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Shorten failed. (%wZ)",
						VolumeName.Get());

					__leave;
				}
			}
		}
		else if (25 <= VolumeName.GetLenCh())
		{
			if (!VolumeName.Shorten(25))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Shorten failed. (%wZ)",
					VolumeName.Get());

				__leave;
			}

			if (L'\\' == *(VolumeName.GetString() + VolumeName.GetLenCh() - 1))
			{
				// \Device\HarddiskVolume11\ 
				if (!VolumeName.Shorten(24))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Shorten failed. (%wZ)",
						VolumeName.Get());

					__leave;
				}
			}
			else
			{
				// \Device\HarddiskVolume1\X
				if (!VolumeName.Shorten(23))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Shorten failed. (%wZ)",
						VolumeName.Get());

					__leave;
				}
			}
		}

		ntStatus = FltGetVolumeFromName(
			CMinifilter::gFilterHandle,
			VolumeName.Get(),
			&pFltVolume
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltGetVolumeFromName failed. (%x)",
				ntStatus);

			__leave;
		}

		if (!pFltVolume)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"NULL == pFltVolume");
			__leave;
		}

		ntStatus = FltGetVolumeInstanceFromName(
			CMinifilter::gFilterHandle,
			pFltVolume,
			NULL,
			&ms_pFltInstance
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltGetVolumeInstanceFromName failed. (%x)",
				ntStatus);

			__leave;
		}

		if (!ms_pFltInstance)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"NULL == ms_pFltInstance");
			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (pFltVolume)
		{
			FltObjectDereference(pFltVolume);
			pFltVolume = NULL;
		}
	}

	return bRet;
}

BOOLEAN
CLog::InitSectorSize()
{
	BOOLEAN					bRet = FALSE;

	NTSTATUS				ntStatus = STATUS_UNSUCCESSFUL;
	PFLT_VOLUME				pFltVolume = NULL;
	UCHAR					volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES) + 512] = { 0 };
	PFLT_VOLUME_PROPERTIES	volProp = (PFLT_VOLUME_PROPERTIES)volPropBuffer;
	ULONG					retLen = 0;

	CKrnlStr				VolumeName;


	__try
	{
		if (!VolumeName.Set(ms_pLogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Set failed. (%wZ)",
				ms_pLogDir->Get());

			__leave;
		}

		if (24 == VolumeName.GetLenCh())
		{
			if (L'\\' == *(VolumeName.GetString() + VolumeName.GetLenCh() - 1))
			{
				// \Device\HarddiskVolume1\ 
				if (!VolumeName.Shorten(23))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Shorten failed. (%wZ)",
						VolumeName.Get());

					__leave;
				}
			}
		}
		else if (25 <= VolumeName.GetLenCh())
		{
			if (!VolumeName.Shorten(25))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Shorten failed. (%wZ)",
					VolumeName.Get());

				__leave;
			}

			if (L'\\' == *(VolumeName.GetString() + VolumeName.GetLenCh() - 1))
			{
				// \Device\HarddiskVolume11\ 
				if (!VolumeName.Shorten(24))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Shorten failed. (%wZ)",
						VolumeName.Get());

					__leave;
				}
			}
			else
			{
				// \Device\HarddiskVolume1\X
				if (!VolumeName.Shorten(23))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"VolumeName.Shorten failed. (%wZ)",
						VolumeName.Get());

					__leave;
				}
			}
		}

		ntStatus = FltGetVolumeFromName(
			CMinifilter::gFilterHandle,
			VolumeName.Get(),
			&pFltVolume
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltGetVolumeFromName failed. (%x)",
				ntStatus);

			__leave;
		}

		if (!pFltVolume)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"NULL == pFltVolume");
			__leave;
		}

		//
		//  Always get the volume properties, so I can get a sector size
		//
		ntStatus = FltGetVolumeProperties(
			pFltVolume,
			volProp,
			sizeof(volPropBuffer),
			&retLen
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltGetVolumeProperties failed. (%x)",
				ntStatus);

			__leave;
		}

		//
		//  Save the sector size in the context for later use.  Note that
		//  we will pick a minimum sector size if a sector size is not
		//  specified.
		//

		ms_ulSectorSize = max(volProp->SectorSize, MIN_SECTOR_SIZE);

		bRet = TRUE;
	}
	__finally
	{
		if (pFltVolume)
		{
			FltObjectDereference(pFltVolume);
			pFltVolume = NULL;
		}
	}

	return bRet;
}

BOOLEAN
CLog::InitLogDir(
__in CKrnlStr * pLogPath
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	LogDir;


	__try
	{
		if (!pLogPath)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		if (!CFileName::GetParentPath(pLogPath, &LogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"GetParentPath failed. (%wZ)",
				pLogPath->Get());

			__leave;
		}		

		if (!CFile::CreateDir(&LogDir, ms_pFltInstance))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"CreateDir failed. (%wZ)",
				LogDir.Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CLog::IsOldLogFile(
__in CKrnlStr * pName
)
{
	BOOLEAN			bRet = FALSE;

	LARGE_INTEGER	snow = { 0 };
	LARGE_INTEGER	now = { 0 };
	TIME_FIELDS		nowFields = { 0 };
	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;
	TIME_FIELDS		oldFields = { 0 };

	CKrnlStr		Temp;


	__try
	{
		if (!pName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		if (10 > pName->GetLenCh())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"pName error. (%wZ)",
				pName->Get());

			__leave;
		}

		KeQuerySystemTime(&snow);
		ExSystemTimeToLocalTime(&snow, &now);
		RtlTimeToTimeFields(&now, &nowFields);

		// 年
		if (!Temp.Set(pName->GetString(), 4))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"Temp.Set failed. (%wZ)",
				pName->Get());

			__leave;
		}

		ntStatus = RtlUnicodeStringToInteger(Temp.Get(), 0, (PULONG)&oldFields.Year);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlUnicodeStringToInteger failed. (%wZ)",
				Temp.Get());

			__leave;
		}

		// 月
		if (!Temp.Set(pName->GetString() + 5, 2))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"Temp.Set failed. (%wZ)",
				pName->Get());

			__leave;
		}

		ntStatus = RtlUnicodeStringToInteger(Temp.Get(), 0, (PULONG)&oldFields.Month);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlUnicodeStringToInteger failed. (%wZ)",
				Temp.Get());

			__leave;
		}

		// 日
		if (!Temp.Set(pName->GetString() + 8, 2))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"Temp.Set failed. (%wZ)",
				pName->Get());

			__leave;
		}

		ntStatus = RtlUnicodeStringToInteger(Temp.Get(), 0, (PULONG)&oldFields.Day);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlUnicodeStringToInteger failed. (%wZ)",
				Temp.Get());

			__leave;
		}

		if (nowFields.Year == oldFields.Year)
		{
			// Now 2016  Old 2016
			if (1 == nowFields.Month - oldFields.Month)
			{
				// Now 2016-09  Old 2016-08
				if (nowFields.Day > oldFields.Day)
				{
					// Now 2016-09-10  Old 2016-08-09
					bRet = TRUE;
				}
			}
			else if (2 <= nowFields.Month - oldFields.Month)
			{
				// Now 2016-09  Old 2016-07
				bRet = TRUE;
			}
		}
		else if (1 == nowFields.Year - oldFields.Year)
		{
			// Now 2016  Old 2015
			if (1 == nowFields.Month && 12 == oldFields.Month)
			{
				// Now 2016-01  Old 2015-12
				if (nowFields.Day > oldFields.Day)
				{
					// Now 2016-01-10  Old 2015-12-09
					bRet = TRUE;
				}
			}
			else
				bRet = TRUE;
		}
		else
		{
			// Now 2016  Old 2014
			bRet = TRUE;
		}
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CLog::IsSameDate(
__in CKrnlStr * pLog
)
{
	BOOLEAN		bRet = TRUE;

	CKrnlStr	FileName;


	__try
	{
		if (!pLog)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		if (!FileName.Set(ms_pLogFile->GetString() + (ms_pLogFile->GetLenCh() - 30), 11))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FileName.Set failed. (%wZ)",
				ms_pLogFile->Get());

			__leave;
		}

		bRet = FileName.Equal(pLog->GetString() + 6, 11, TRUE);
	}
	__finally
	{
		;
	}

	return bRet;
}
