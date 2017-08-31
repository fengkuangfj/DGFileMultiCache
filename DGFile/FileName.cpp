
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：FileName.cpp

摘  要：

作  者：岳翔

--*/

#include "Stdafx.h"
#include "FileName.h"

#pragma warning(disable : 4509)

BOOLEAN
CFileName::GetVolDevNameByQueryObj(
	__in	CKrnlStr * pSymName,
	__out	CKrnlStr * pDevName
)
{
	BOOLEAN				bRet = FALSE;

	OBJECT_ATTRIBUTES	Oa = { 0 };
	NTSTATUS			ntStatus = STATUS_UNSUCCESSFUL;
	HANDLE				Handle = NULL;


	__try
	{
		if (!pSymName || !pDevName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. (%p) (%p)",
				pSymName, pDevName);

			__leave;
		}

		InitializeObjectAttributes(
			&Oa,
			pSymName->Get(),
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
		);

		ntStatus = ZwOpenSymbolicLinkObject(
			&Handle,
			GENERIC_READ,
			&Oa
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ZwOpenSymbolicLinkObject failed. (%x) (%wZ)",
				ntStatus, pSymName->Get());

			__leave;
		}

		if (!pDevName->Init())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pDevName.Init failed");
			__leave;
		}

		ntStatus = ZwQuerySymbolicLinkObject(
			Handle,
			pDevName->Get(),
			NULL
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ZwQuerySymbolicLinkObject failed. (%x) (%wZ)",
				ntStatus, pSymName->Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (Handle)
		{
			ZwClose(Handle);
			Handle = NULL;
		}
	}

	return bRet;
}

BOOLEAN
CFileName::AppToSym(
	__in CKrnlStr * pAppName,
	__in CKrnlStr * pSymName
)
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!pAppName || !pSymName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. (%p) (%p)",
				pAppName, pSymName);

			__leave;
		}

		if (!pSymName->Set(L"\\??\\", (USHORT)wcslen(L"\\??\\")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pSymName->Set failed");
			__leave;
		}

		if (!pSymName->Append(pAppName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pSymName->Append failed. (%wZ)",
				pAppName->Get());

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
CFileName::AppToDev(
	__in	CKrnlStr * pAppName,
	__inout CKrnlStr * pDevName
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	SymName;


	__try
	{
		if (!pAppName || !pDevName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. (%p) (%p)",
				pAppName, pDevName);

			__leave;
		}

		if (!AppToSym(pAppName, &SymName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"AppToSym failed. (%wZ)",
				pAppName);

			__leave;
		}

		if (!SymToDev(&SymName, pDevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"SymToDev failed. (%wZ)",
				SymName.Get());

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
CFileName::SymToDev(
	__in	CKrnlStr * pSymName,
	__inout CKrnlStr * pDevName
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	SymPrePart;
	CKrnlStr	PostPart;


	__try
	{
		if (!pSymName || !pDevName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. (%p) (%p)",
				pSymName, pDevName);

			__leave;
		}

		if (!SymPrePart.Set(pSymName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"SymPrePart.Set failed. (%wZ)",
				pSymName->Get());

			__leave;
		}

		if (4 == SymPrePart.GetLenCh())
		{
			// \?\c
			if (!SymPrePart.Append(L":", (USHORT)wcslen(L":")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"SymPrePart.Append failed. (%wZ)",
					SymPrePart.Get());

				__leave;
			}
		}
		else if (6 <= SymPrePart.GetLenCh())
		{
			// \?\c:X
			if (!PostPart.Set(SymPrePart.GetString() + 5, SymPrePart.GetLenCh() - 5))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"PostPart.Set failed. (%wZ)",
					SymPrePart.Get());

				__leave;
			}

			if (!SymPrePart.Shorten(5))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"SymPrePart.Shorten failed. (%wZ)",
					SymPrePart.Get());

				__leave;
			}
		}

		if (!GetVolDevNameByQueryObj(&SymPrePart, pDevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"GetVolDevNameByQueryObj failed. (%wZ)",
				SymPrePart.Get());

			__leave;
		}

		if (PostPart.GetLenCh())
		{
			if (!pDevName->Append(&PostPart))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pDevName->Append failed. (%wZ) (%wZ)",
					pDevName->Get(), PostPart.Get());

				__leave;
			}
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
CFileName::GetParentPath(
	__in	CKrnlStr * pPath,
	__out	CKrnlStr * pParentPath
)
{
	BOOLEAN		bRet = FALSE;

	PWCHAR		pwchPostion = NULL;
	PWCHAR		pwchPostionStart = NULL;
	PWCHAR		pwchPostionEnd = NULL;
	ULONG		ulCount = 0;


	__try
	{
		if (!pPath || !pParentPath)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pPath(%p) pParentPath(%p)",
				pPath, pParentPath);

			__leave;
		}

		if (!pParentPath->Set(pPath))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pParentPath->Set failed. Path(%wZ)",
				pPath->Get());

			__leave;
		}

		pwchPostionStart = pParentPath->GetString();
		pwchPostionEnd = pwchPostion = pParentPath->GetString() + pParentPath->GetLenCh() - 1;

		for (; pwchPostion >= pwchPostionStart; pwchPostion--)
		{
			if (L'\\' == *pwchPostion && pwchPostion != pwchPostionEnd)
				ulCount++;

			if (1 == ulCount)
			{
				if (!pParentPath->Shorten((USHORT)(pwchPostion - pwchPostionStart)))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pParentPath->Shorten failed. Path(%wZ)",
						pParentPath->Get());

					__leave;
				}

				bRet = TRUE;
				__leave;
			}
			else
			{
				if (2 <= ulCount)
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ulCount error. ulCount(%d) Path(%wZ)",
						ulCount, pParentPath->Get());

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
CFileName::SystemRootToDev(
	__in	CKrnlStr* pFileName,
	__inout CKrnlStr* pFileNameDev
)
{
	BOOLEAN bRet = FALSE;

	CKrnlStr FileNameSystemRoot;
	CKrnlStr FileNameTmpLong;
	CKrnlStr FileNameTmpShort;
	CKrnlStr FileNameWindows;

	PWCHAR	pPosition = NULL;


	__try
	{
		if (!pFileName || !pFileNameDev)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pFileName(%p) pFileNameDev(%p)",
				pFileName, pFileNameDev);

			__leave;
		}

		if (!FileNameSystemRoot.Set(pFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameSystemRoot.Set failed. File(%wZ)",
				pFileName->Get());

			__leave;
		}

		if (!FileNameSystemRoot.Shorten(11))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameSystemRoot.Shorten failed. File(%wZ)",
				FileNameSystemRoot.Get());

			__leave;
		}

		if (!CFileName::ConvertByZwQuerySymbolicLinkObject(&FileNameSystemRoot, &FileNameTmpLong))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ConvertByZwQuerySymbolicLinkObject failed. File(%wZ)",
				FileNameSystemRoot.Get());

			__leave;
		}

		if (!FileNameTmpShort.Set(&FileNameTmpLong))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameTmpShort.Set failed. File(%wZ)",
				FileNameTmpLong.Get());

			__leave;
		}

		for (pPosition = FileNameTmpShort.GetString() + FileNameTmpShort.GetLenCh() - 1; pPosition >= FileNameTmpShort.GetString(); pPosition--)
		{
			if (*pPosition == L'\\')
				break;
		}

		if (!FileNameWindows.Set(pPosition, FileNameTmpShort.GetLenCh() - (USHORT)(pPosition - FileNameTmpShort.GetString())))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameWindows.Set failed. Windows(%lS)",
				pPosition);

			__leave;
		}

		if (!FileNameTmpShort.Shorten((USHORT)(pPosition - FileNameTmpShort.GetString())))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameTmpShort.Shorten failed. File(%wZ)",
				FileNameTmpShort.Get());

			__leave;
		}

		if (!CFileName::ConvertByZwQuerySymbolicLinkObject(&FileNameTmpShort, pFileNameDev))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ConvertByZwQuerySymbolicLinkObject failed. File(%wZ)",
				FileNameTmpShort.Get());

			__leave;
		}

		if (!pFileNameDev->Append(&FileNameWindows))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFileNameDev->Append failed. File(%wZ)",
				pFileNameDev->Get());

			__leave;
		}

		if (!pFileNameDev->Append(pFileName->GetString() + 11, pFileName->GetLenCh() - 11))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFileNameDev->Append failed. File(%wZ) File(%wZ)",
				pFileNameDev->Get(), pFileName->Get());

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
CFileName::ConvertByZwQuerySymbolicLinkObject(
	__in	CKrnlStr* pFileName,
	__inout CKrnlStr* pNewFileName
)
{
	BOOLEAN				bRet = FALSE;

	OBJECT_ATTRIBUTES	Oa = { 0 };
	NTSTATUS			ntStatus = STATUS_UNSUCCESSFUL;
	HANDLE				Handle = NULL;


	__try
	{
		if (!pFileName || !pNewFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pFileName(%p) pNewFileName(%p)",
				pFileName, pNewFileName);

			__leave;
		}

		// 最后不带"\\"
		InitializeObjectAttributes(
			&Oa,
			pFileName->Get(),
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
		);

		ntStatus = ZwOpenSymbolicLinkObject(
			&Handle,
			GENERIC_READ,
			&Oa
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ZwOpenSymbolicLinkObject failed. File(%wZ) (%x)",
				pFileName->Get(), ntStatus);

			__leave;
		}

		if (!pNewFileName->Init())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pNewFileName->Init failed");
			__leave;
		}

		ntStatus = ZwQuerySymbolicLinkObject(
			Handle,
			pNewFileName->Get(),
			NULL
		);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ZwQuerySymbolicLinkObject failed. File(%wZ) (%x)",
				pFileName->Get(), ntStatus);

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (Handle)
		{
			ZwClose(Handle);
			Handle = NULL;
		}
	}

	return bRet;
}

/*
* 函数说明：
*		拼接文件名
*
* 参数：
*		pDirPath		文件所在文件夹
*		pFileName		文件名
*		pFilePath		文件全路径
*
* 返回值：
*		TRUE	成功
*		FALSE	失败
*
* 备注：
*		无
*/
BOOLEAN
CFileName::SpliceFilePath(
	__in	CKrnlStr*	pDirPath,
	__in	CKrnlStr*	pFileName,
	__inout	CKrnlStr*	pFilePath
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	TmpFileName;


	__try
	{
		if (!pDirPath || !pFileName || !pFilePath)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pDirPath(%p) pFileName(%p) pFilePath(%p)", pDirPath, pFileName, pFilePath);
			__leave;
		}


		if (!pFilePath->Set(pDirPath))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFilePath->Set failed. File(%wZ)", pDirPath->Get());
			__leave;
		}

		// 开始判断FileName是否以'\'结束
		if (!TmpFileName.Set(pFilePath->GetString() + pFilePath->GetLenCh() - 1, pFilePath->GetLenCh() - (pFilePath->GetLenCh() - 1)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"TmpFileName.Set failed. File(%wZ)", pFilePath->Get());
			__leave;
		}

		if (TmpFileName.Equal(L"\\", (USHORT)wcslen(L"\\"), FALSE))
		{
			if (!pFilePath->Shorten(pFilePath->GetLenCh() - 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFilePath->Shorten failed. File(%wZ)", pFilePath->Get());
				__leave;
			}
		}

		// 开始判断pFileName是否以'\'开始
		if (!TmpFileName.Set(pFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"TmpFileName.Set failed. File(%wZ)", pFileName->Get());
			__leave;
		}

		if (!TmpFileName.Shorten(1))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"TmpFileName.Shorten failed. File(%wZ)", TmpFileName.Get());
			__leave;
		}

		if (!TmpFileName.Equal(L"\\", (USHORT)wcslen(L"\\"), FALSE))
		{
			if (!pFilePath->Append(L"\\", (USHORT)wcslen(L"\\")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFilePath->Append failed. File(%wZ)", pFilePath->Get());
				__leave;
			}
		}

		if (!pFilePath->Append(pFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFilePath->Append failed. File(%wZ)", pFileName->Get());
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
