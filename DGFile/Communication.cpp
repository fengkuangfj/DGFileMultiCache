
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：Communication.cpp

摘  要：

作  者：岳翔

--*/

#include "stdafx.h"
#include "Communication.h"

#pragma warning(disable : 4509)

COMM_INFO	CCommunication::ms_CommInfo = { 0 };

CCommunication::CCommunication()
{
	;
}

CCommunication::~CCommunication()
{
	;
}

BOOLEAN
CCommunication::Init(
__in PFLT_FILTER pFlt
)
{
	BOOLEAN					bRet = FALSE;

	CKrnlStr				kstrPortName;

	NTSTATUS				ntStatus = STATUS_UNSUCCESSFUL;
	PSECURITY_DESCRIPTOR	pSd = NULL;
	OBJECT_ATTRIBUTES		Ob = { 0 };

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		// 传入参数检测
		if (!pFlt)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (!kstrPortName.Set(PortName, (USHORT)wcslen(PortName)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"PortName.Set failed");
			__leave;
		}

		ntStatus = FltBuildDefaultSecurityDescriptor(&pSd, FLT_PORT_ALL_ACCESS);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltBuildDefaultSecurityDescriptor failed (%x)",
				ntStatus);

			__leave;
		}

		InitializeObjectAttributes(
			&Ob,
			kstrPortName.Get(),
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
			NULL,
			pSd
			);

		ntStatus = FltCreateCommunicationPort(
			pFlt,
			&ms_CommInfo.pSeverPort,
			&Ob,
			NULL,
			(PFLT_CONNECT_NOTIFY)CommKmConnectNotify,
			(PFLT_DISCONNECT_NOTIFY)CommKmDisconnectNotify,
			(PFLT_MESSAGE_NOTIFY)CommKmMessageNotify,
			1
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateCommunicationPort failed (%x)",
				ntStatus);

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (pSd)
		{
			FltFreeSecurityDescriptor(pSd);
			pSd = NULL;
		}
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return bRet;
}

VOID
CCommunication::Unload()
{
	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	if (ms_CommInfo.pSeverPort)
	{
		FltCloseCommunicationPort(ms_CommInfo.pSeverPort);
		ms_CommInfo.pSeverPort = NULL;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");
}

NTSTATUS
CCommunication::CommKmConnectNotify(
__in			PFLT_PORT	pClientPort,
__in_opt		PVOID		pServerPortCookie,
__in_bcount_opt(ulSizeOfContext) PVOID lpConnectionContext,
__in			ULONG		ulSizeOfContext,
__deref_out_opt PVOID*		pConnectionPortCookie
)
{
	NTSTATUS		Status = STATUS_UNSUCCESSFUL;

	CProcTbl		ProcTbl;
	CLog			Log;

	CKrnlStr		ProcPath;
	CKrnlStr		LogDir;

	ULONG			ulPid = 0;


	UNREFERENCED_PARAMETER(pServerPortCookie);
	UNREFERENCED_PARAMETER(lpConnectionContext);
	UNREFERENCED_PARAMETER(ulSizeOfContext);
	UNREFERENCED_PARAMETER(pConnectionPortCookie);

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		if (ms_CommInfo.pClientPort)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"already connected");
			Status = STATUS_SUCCESS;
			__leave;
		}

		ms_CommInfo.pClientPort = pClientPort;
		ulPid = (ULONG)PsGetCurrentProcessId();

		if (!ProcTbl.GetProcPath(ulPid, &ProcPath))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ProcTbl.GetProcessPath failed. Pid(%d)",
				ulPid);

			__leave;
		}

		if (!CFileName::GetParentPath(&ProcPath, &LogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.GetParentPath failed. Path(%wZ)",
				ProcPath.Get());

			__leave;
		}

		if (!LogDir.Append(L"\\log", (USHORT)wcslen(L"\\log")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"LogDir.Append failed. Dir(%wZ)",
				LogDir.Get());

			__leave;
		}

		if (!Log.SetLogDir(&LogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Log.SetLogDir failed. Dir(%wZ)",
				LogDir.Get());

			__leave;
		}

		Status = STATUS_SUCCESS;
	}
	__finally
	{
		if (STATUS_SUCCESS != Status)
			ms_CommInfo.pClientPort = NULL;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return Status;
}

VOID
CCommunication::CommKmDisconnectNotify(
__in_opt PVOID lpConnectionCookie
)
{
	UNREFERENCED_PARAMETER(lpConnectionCookie);

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		if (CMinifilter::CheckEnv(MINIFILTER_ENV_TYPE_FLT_FILTER) && ms_CommInfo.pClientPort)
			FltCloseClientPort(CMinifilter::gFilterHandle, &ms_CommInfo.pClientPort);

		ms_CommInfo.pClientPort = NULL;
	}
	__finally
	{
		;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return;
}

NTSTATUS
CCommunication::CommKmMessageNotify(
__in_opt	PVOID	lpPortCookie,
__in_bcount_opt(ulInputBufferLength) PVOID lpInputBuffer,
__in		ULONG	ulInputBufferLength,
__out_bcount_part_opt(ulOutputBufferLength, *pulReturnOutputBufferLength) PVOID lpOutputBuffer,
__in		ULONG	ulOutputBufferLength,
__out		PULONG	pulReturnOutputBufferLength
)
{
	NTSTATUS			ntStatus = STATUS_UNSUCCESSFUL;

	ESafeCommandType	cmdType;
	PSystemApi32Use		pSyatem = { 0 };

	UNREFERENCED_PARAMETER(lpPortCookie);
	UNREFERENCED_PARAMETER(ulInputBufferLength);
	UNREFERENCED_PARAMETER(lpOutputBuffer);
	UNREFERENCED_PARAMETER(ulOutputBufferLength);
	UNREFERENCED_PARAMETER(pulReturnOutputBufferLength);

	__try
	{
		if (!ms_CommInfo.pSeverPort || !ms_CommInfo.pClientPort)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"comm parameter error");
			__leave;
		}

		cmdType = ((PCommandMsg)lpInputBuffer)->CommandType;
		switch (cmdType)
		{
		case eSetSystemUse:
		{
			pSyatem = (PSystemApi32Use)(((PCommandMsg)lpInputBuffer)->MsgInfo);

			KeEnterCriticalRegion();

			if (!strlen(CMinifilter::gSafeData.szbtKey))
			{
				RtlCopyMemory(&CMinifilter::gSafeData.SystemUser, pSyatem, sizeof(SystemApi32Use));
				RtlCopyMemory(CMinifilter::gSafeData.szbtKey, pSyatem->Key, KEY_LEN);

				if (strlen(CMinifilter::gSafeData.szbtKey))
				{
					if (CMinifilter::CheckDebugFlag(DEBUG_FLAG_PRINT_DOGID_AND_KEY))
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"[DogID] %d - 0x%x", CMinifilter::gSafeData.SystemUser.DogID);
						KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"[Key] %hs", CMinifilter::gSafeData.szbtKey);
					}

					CMinifilter::ms_bSafeDataReady = TRUE;
				}
				else
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"DogID and Key error");
			}
			else
				KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"DogID and Key already set");

			KeLeaveCriticalRegion();

			KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"ms_bSafeDataReady");

			ntStatus = STATUS_SUCCESS;
			break;
		}
		case eSetProcAuthentic:
			break;
		default:
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"cmdType error. cmdType(%d)",
				cmdType);

			__leave;
		}
		}
	}
	__finally
	{
		;
	}

	return ntStatus;
}
