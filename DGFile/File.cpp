
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：File.cpp

摘  要：

作  者：岳翔

--*/

#include "Stdafx.h"
#include "File.h"

#pragma warning(disable : 4509)
#pragma warning(disable : 4127)

CKrnlStr	*	CFile::ms_pSystemRoot = NULL;
CKrnlStr	*	CFile::ms_pPageFileSys = NULL;
CKrnlStr	*	CFile::ms_pHiberFilSys = NULL;
CKrnlStr	*	CFile::ms_pSwapFileSys = NULL;
CKrnlStr	*	CFile::ms_pPassthroughPathEndsWithList = NULL;
ULONG			CFile::ms_ulPassthroughPathEndsWithCount = 0;
CKrnlStr	*	CFile::ms_pPassthroughPathContainsList = NULL;
ULONG			CFile::ms_ulPassthroughPathContainsCount = 0;

CFile::CFile()
{
	;
}

CFile::~CFile()
{
	;
}

BOOLEAN
CFile::IsShadowCopy(
	__in CKrnlStr * pFileName
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	FileName;
	CKrnlStr	FileNameCmp;


	__try
	{
		if (!pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if (!FileName.Set(L"\\Device\\HarddiskVolumeShadowCopy", (USHORT)wcslen(L"\\Device\\HarddiskVolumeShadowCopy")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.Set failed");
			__leave;
		}

		if (pFileName->GetLenCh() < FileName.GetLenCh())
			__leave;
		else if (pFileName->GetLenCh() > FileName.GetLenCh())
		{
			if (!FileNameCmp.Set(pFileName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameCmp.Set failed. File(%wZ)",
					pFileName->Get());

				__leave;
			}

			if (!FileNameCmp.Shorten(FileName.GetLenCh()))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameCmp.Shorten failed");
				__leave;
			}

			bRet = FileNameCmp.Equal(&FileName, TRUE);
		}
		else
			bRet = pFileName->Equal(&FileName, TRUE);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CFile::Init()
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	FileName;
	CKrnlStr	VolumeFileName;

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		ms_pSystemRoot = new(FILE_TAG)CKrnlStr;

		if (!FileName.Set(L"\\systemroot", (USHORT)wcslen(L"\\systemroot")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.Set failed");
			__leave;
		}

		if (!CFileName::SystemRootToDev(&FileName, ms_pSystemRoot))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"SystemRootToDev failed. File(%wZ)",
				FileName.Get());

			__leave;
		}

		KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"ms_pSystemRoot (%wZ)",
			ms_pSystemRoot->Get());

		if (!VolumeFileName.Set(ms_pSystemRoot))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolumeFileName.Set failed. File(%wZ)",
				ms_pSystemRoot->Get());

			__leave;
		}

		if (!VolumeFileName.Shorten(ms_pSystemRoot->GetLenCh() - (USHORT)wcslen(L"windows")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolumeFileName.Shorten failed. File(%wZ)",
				VolumeFileName.Get());

			__leave;
		}

		ms_pPageFileSys = new(FILE_TAG)CKrnlStr;

		if (!ms_pPageFileSys->Set(&VolumeFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_pPageFileSys->Set failed. File(%wZ)",
				VolumeFileName.Get());

			__leave;
		}

		if (!ms_pPageFileSys->Append(L"pagefile.sys", (USHORT)wcslen(L"pagefile.sys")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_pPageFileSys->Append failed. File(%wZ)",
				ms_pPageFileSys->Get());

			__leave;
		}

		ms_pHiberFilSys = new(FILE_TAG)CKrnlStr;

		if (!ms_pHiberFilSys->Set(&VolumeFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_pHiberFilSys->Set failed. File(%wZ)",
				VolumeFileName.Get());

			__leave;
		}

		if (!ms_pHiberFilSys->Append(L"hiberfil.sys", (USHORT)wcslen(L"hiberfil.sys")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_pHiberFilSys->Append failed. File(%wZ)",
				ms_pHiberFilSys->Get());

			__leave;
		}

		ms_pSwapFileSys = new(FILE_TAG)CKrnlStr;

		if (!ms_pSwapFileSys->Set(&VolumeFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_pSwapFileSys->Set failed. File(%wZ)",
				VolumeFileName.Get());

			__leave;
		}

		if (!ms_pSwapFileSys->Append(L"swapfile.sys", (USHORT)wcslen(L"swapfile.sys")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ms_pSwapFileSys->Append failed. File(%wZ)",
				ms_pSwapFileSys->Get());

			__leave;
		}

		if (!InitPassthroughPathEndsWithList())
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"InitPassthroughPathEndsWithList failed");

		if (!InitPassthroughPathContainsList())
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"InitPassthroughPathContainsList failed");

		bRet = TRUE;
	}
	__finally
	{
		if (!bRet)
		{
			if (!Unload())
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Unload failed");
		}
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return bRet;
}

BOOLEAN
CFile::Unload()
{
	BOOLEAN bRet = FALSE;

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		delete ms_pSystemRoot;
		ms_pSystemRoot = NULL;

		delete ms_pPageFileSys;
		ms_pPageFileSys = NULL;

		delete ms_pHiberFilSys;
		ms_pHiberFilSys = NULL;

		delete ms_pSwapFileSys;
		ms_pSwapFileSys = NULL;

		delete[] ms_pPassthroughPathEndsWithList;
		ms_pPassthroughPathEndsWithList = NULL;

		delete[] ms_pPassthroughPathContainsList;
		ms_pPassthroughPathContainsList = NULL;

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return bRet;
}

BOOLEAN
CFile::IsHiberFilSys(
	__in CKrnlStr* pFileName
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	strTemp;


	__try
	{
		if (!pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if (pFileName->GetLenCh() > ms_pHiberFilSys->GetLenCh())
		{
			if (!strTemp.Set(pFileName->GetString(), ms_pHiberFilSys->GetLenCh()))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"strTemp.Set failed. (%wZ) (%wZ)",
					pFileName->Get(), ms_pHiberFilSys->Get());

				__leave;
			}

			bRet = strTemp.Equal(ms_pHiberFilSys, TRUE);
		}
		else
			bRet = pFileName->Equal(ms_pHiberFilSys, TRUE);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CFile::IsSwapFileSys(
	__in CKrnlStr* pFileName
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	strTemp;


	__try
	{
		if (!pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if (pFileName->GetLenCh() > ms_pSwapFileSys->GetLenCh())
		{
			if (!strTemp.Set(pFileName->GetString(), ms_pSwapFileSys->GetLenCh()))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"strTemp.Set failed. (%wZ) (%wZ)",
					pFileName->Get(), ms_pSwapFileSys->Get());

				__leave;
			}

			bRet = strTemp.Equal(ms_pSwapFileSys, TRUE);
		}
		else
			bRet = pFileName->Equal(ms_pSwapFileSys, TRUE);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CFile::IsPageFileSys(
	__in CKrnlStr* pFileName
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	strTemp;


	__try
	{
		if (!pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if (pFileName->GetLenCh() > ms_pPageFileSys->GetLenCh())
		{
			if (!strTemp.Set(pFileName->GetString(), ms_pPageFileSys->GetLenCh()))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"strTemp.Set failed. (%wZ) (%wZ)",
					pFileName->Get(), ms_pPageFileSys->Get());

				__leave;
			}

			bRet = strTemp.Equal(ms_pPageFileSys, TRUE);
		}
		else
			bRet = pFileName->Equal(ms_pPageFileSys, TRUE);
	}
	__finally
	{
		;
	}

	return bRet;
}


BOOLEAN
CFile::IsExpression(
	__in CKrnlStr* pFileName
)
{
	BOOLEAN bRet = FALSE;

	PWCHAR	pPosition = NULL;


	__try
	{
		if (!pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		for (pPosition = pFileName->GetString(); pPosition < pFileName->GetString() + pFileName->GetLenCh(); pPosition++)
		{
			if (L'*' == *pPosition)
			{
				bRet = TRUE;
				__leave;
			}
		}
	}
	__finally
	{
		;
	}

	return bRet;
}



BOOLEAN
CFile::CreateDir(
	__in CKrnlStr * pLogDir
)
{
	BOOLEAN				bRet = FALSE;

	OBJECT_ATTRIBUTES	Oa = { 0 };
	NTSTATUS			ntStatus = STATUS_UNSUCCESSFUL;
	HANDLE				hDir = NULL;
	IO_STATUS_BLOCK		Iosb = { 0 };

	CKrnlStr			ParentDir;


	__try
	{
		if (!pLogDir)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		InitializeObjectAttributes(
			&Oa,
			pLogDir->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
		);

		ntStatus = ZwCreateFile(
			&hDir,
			GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
			&Oa,
			&Iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			NULL,
			FILE_OPEN_IF,
			FILE_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
			NULL,
			0
		);
		if (!NT_SUCCESS(ntStatus))
		{
			if (STATUS_OBJECT_PATH_NOT_FOUND == ntStatus)
			{
				if (!CFileName::GetParentPath(pLogDir, &ParentDir))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"GetParentPath failed. (%wZ)",
						pLogDir->Get());

					__leave;
				}

				if (!CreateDir(&ParentDir))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CreateDir failed. (%wZ)",
						ParentDir.Get());

					__leave;
				}

				InitializeObjectAttributes(
					&Oa,
					pLogDir->Get(),
					OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
					NULL,
					NULL
				);

				ntStatus = ZwCreateFile(
					&hDir,
					GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
					&Oa,
					&Iosb,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					NULL,
					FILE_OPEN_IF,
					FILE_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
					NULL,
					0
				);
				if (!NT_SUCCESS(ntStatus))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. (%x) (%wZ)",
						ntStatus, pLogDir->Get());

					__leave;
				}
			}
			else
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. (%x) (%wZ)",
					ntStatus, pLogDir->Get());

				__leave;
			}
		}

		bRet = TRUE;
	}
	__finally
	{
		if (hDir)
		{
			ZwClose(hDir);
			hDir = NULL;
		}
	}

	return bRet;
}


BOOLEAN
CFile::InitPassthroughPathEndsWithList()
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	Key;
	CKrnlStr	Value;


	__try
	{
		if (!Key.Set(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\services\\DGFile", (USHORT)wcslen(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\services\\DGFile")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Key.Set failed");
			__leave;
		}

		if (!Value.Set(L"PassthroughPathEndsWith", (USHORT)wcslen(L"PassthroughPathEndsWith")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Value.Set failed");
			__leave;
		}

// 		bRet = CDGFile::InitList(
// 			&Key,
// 			&Value,
// 			&ms_pPassthroughPathEndsWithList,
// 			&ms_ulPassthroughPathEndsWithCount
// 		);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CFile::InitPassthroughPathContainsList()
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	Key;
	CKrnlStr	Value;


	__try
	{
		if (!Key.Set(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\services\\DGFile", (USHORT)wcslen(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\services\\DGFile")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Key.Set failed");
			__leave;
		}

		if (!Value.Set(L"PassthroughPathContains", (USHORT)wcslen(L"PassthroughPathContains")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Value.Set failed");
			__leave;
		}

// 		bRet = CDGFile::InitList(
// 			&Key,
// 			&Value,
// 			&ms_pPassthroughPathContainsList,
// 			&ms_ulPassthroughPathContainsCount
// 		);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CFile::IsInPassthroughPathEndsWithList(
	__in CKrnlStr * pstrPath
)
{
	BOOLEAN bRet = FALSE;

	ULONG	i = 0;


	__try
	{
		if (!pstrPath)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		for (; i < ms_ulPassthroughPathEndsWithCount; i++)
		{
			if (!ms_pPassthroughPathEndsWithList[i].GetLenCh())
				__leave;

			if (pstrPath->GetLenCh() >= ms_pPassthroughPathEndsWithList[i].GetLenCh())
			{
				if (ms_pPassthroughPathEndsWithList[i].Equal(
					pstrPath->GetString() + (pstrPath->GetLenCh() - ms_pPassthroughPathEndsWithList[i].GetLenCh()),
					ms_pPassthroughPathEndsWithList[i].GetLenCh(),
					TRUE
				))
				{
					bRet = TRUE;
					__leave;
				}
			}
		}
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CFile::IsInPassthroughPathContainsList(
	__in CKrnlStr * pstrPath
)
{
	BOOLEAN bRet = FALSE;

	ULONG	i = 0;


	__try
	{
		if (!pstrPath)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		for (; i < ms_ulPassthroughPathContainsCount; i++)
		{
			if (!ms_pPassthroughPathContainsList[i].GetLenCh())
				__leave;

			if (pstrPath->GetLenCh() >= ms_pPassthroughPathContainsList[i].GetLenCh())
			{
				if (pstrPath->Search(ms_pPassthroughPathContainsList[i].GetString(), TRUE))
				{
					bRet = TRUE;
					__leave;
				}
			}
		}
	}
	__finally
	{
		;
	}

	return bRet;
}
