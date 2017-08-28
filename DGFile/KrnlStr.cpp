
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：KrnlStr.cpp

摘  要：

作  者：岳翔

--*/

#include "stdafx.h"
#include "KrnlStr.h"

#pragma warning(disable : 4127)
#pragma warning(disable : 4509)

CKrnlStr::CKrnlStr()
{
	RtlZeroMemory(&m_Str, sizeof(m_Str));
}

CKrnlStr::~CKrnlStr()
{
	Free();
}

BOOLEAN
CKrnlStr::Alloc(
__in USHORT usLenCh
)
{
	BOOLEAN bRet = FALSE;

	USHORT	usLenB = 0;


	__try
	{
		if (!usLenCh)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		usLenB = usLenCh * sizeof(WCHAR);
		if (usLenB > m_Str.MaximumLength)
		{
			Free();

			m_Str.MaximumLength = usLenB;
			m_Str.Buffer = (PWCHAR)new(KRNLSTR_TAG)CHAR[m_Str.MaximumLength];
		}
		else
			Clean();

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

VOID
CKrnlStr::Free()
{
	if (m_Str.Buffer)
	{
		delete[] m_Str.Buffer;
		m_Str.Buffer = NULL;
	}

	m_Str.Length = 0;
	m_Str.MaximumLength = 0;
}

BOOLEAN
CKrnlStr::Set(
__in CKrnlStr * pCKrnlStr
)
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!pCKrnlStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		bRet = Set(pCKrnlStr->Get());
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::ToUpper()
{
	BOOLEAN bRet = FALSE;

	PWCHAR	pWchPositionCur = NULL;
	PWCHAR	pWchPositionEnd = NULL;


	__try
	{
		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"NULL == m_Str.Buffer");
			__leave;
		}

		pWchPositionCur = m_Str.Buffer;
		pWchPositionEnd = m_Str.Buffer + (m_Str.Length / sizeof(WCHAR));
		for (; pWchPositionCur < pWchPositionEnd; pWchPositionCur++)
		{
			if ((L'a' <= *pWchPositionCur) && (L'z' >= *pWchPositionCur))
				*pWchPositionCur = *pWchPositionCur - (L'a' - L'A');
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
CKrnlStr::Set(
__in PWCHAR pWchStr,
__in USHORT usLenCh
)
{
	BOOLEAN	bRet = FALSE;

	USHORT	usLenBNew = 0;
	USHORT	usLenChNew = 0;


	__try
	{
		if (!pWchStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (pWchStr == m_Str.Buffer)
		{
			bRet = TRUE;
			__leave;
		}

		usLenChNew = usLenCh;
		usLenBNew = usLenChNew * sizeof(WCHAR);
		if (usLenBNew >= m_Str.MaximumLength)
		{
			if (!Alloc(usLenChNew + 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}
		}

		m_Str.Length = usLenBNew;

		RtlCopyMemory(m_Str.Buffer, pWchStr, m_Str.Length);

		m_Str.Buffer[usLenChNew] = UNICODE_NULL;

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Set(
__in PCHAR	pChStr,
__in USHORT usLenCh
)
{
	BOOLEAN		bRet = FALSE;

	USHORT		usLenChNew = 0;
	ANSI_STRING	AnsiString = { 0 };
	NTSTATUS	ntStatus = STATUS_UNSUCCESSFUL;


	__try
	{
		if (!pChStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if ((PWCHAR)pChStr == m_Str.Buffer)
			__leave;

		RtlInitAnsiString(&AnsiString, pChStr);

		usLenChNew = usLenCh + 1;

		do
		{
			if (!Alloc(usLenChNew))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}

			ntStatus = RtlAnsiStringToUnicodeString(&m_Str, &AnsiString, FALSE);
			if (NT_SUCCESS(ntStatus))
				break;

			usLenChNew++;
		} while (TRUE);

		bRet = TRUE;
	}
	__finally
	{
		if (!bRet)
			Free();
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Set(
__in PUNICODE_STRING pUnicodeStr
)
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!pUnicodeStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (pUnicodeStr == &m_Str)
		{
			bRet = TRUE;
			__leave;
		}

		if (!Set(pUnicodeStr->Buffer, pUnicodeStr->Length / sizeof(WCHAR)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Set failed. (%wZ)",
				pUnicodeStr);

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

USHORT
CKrnlStr::GetLenCh()
{
	return m_Str.Length / sizeof(WCHAR);
}

USHORT
CKrnlStr::GetLenB()
{
	return m_Str.Length;
}

PWCHAR
CKrnlStr::GetString()
{
	return m_Str.Buffer;
}

PUNICODE_STRING
CKrnlStr::Get()
{
	return &m_Str;
}

BOOLEAN
CKrnlStr::Equal(
__in CKrnlStr *	pCKrnlStr,
__in BOOLEAN	bIgnoreCase
)
{
	BOOLEAN	bRet = FALSE;


	__try
	{
		if (!pCKrnlStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		bRet = Equal(pCKrnlStr->Get(), bIgnoreCase);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Equal(
__in PWCHAR		pWchStr,
__in USHORT		usLenCh,
__in BOOLEAN	bIgnoreCase
)
{
	BOOLEAN	bRet = FALSE;


	__try
	{
		if (!pWchStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"NULL == m_Str.Buffer");
			__leave;
		}

		if (m_Str.Length != (usLenCh * sizeof(WCHAR)))
			__leave;

		bRet = WcharStrEqual(
			m_Str.Buffer,
			usLenCh,
			pWchStr,
			usLenCh,
			usLenCh,
			bIgnoreCase
			);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Equal(
__in PCHAR		pChStr,
__in USHORT		usLenCh,
__in BOOLEAN	bIgnoreCase
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	StrTmp;


	__try
	{
		if (!pChStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (!StrTmp.Set(pChStr, usLenCh))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"KrnlStr.Set failed. (%hs)",
				pChStr);

			__leave;
		}

		bRet = Equal(&StrTmp, bIgnoreCase);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Equal(
__in PUNICODE_STRING	pUnicodeStr,
__in BOOLEAN			bIgnoreCase
)
{
	BOOLEAN	bRet = FALSE;


	__try
	{
		if (!pUnicodeStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		bRet = Equal(pUnicodeStr->Buffer, pUnicodeStr->Length / sizeof(WCHAR), bIgnoreCase);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Append(
__in CKrnlStr * pCKrnlStr
)
{
	BOOLEAN	bRet = FALSE;


	__try
	{
		if (!pCKrnlStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		bRet = Append(pCKrnlStr->GetString(), pCKrnlStr->GetLenCh());
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Append(
__in PCHAR	pChStr,
__in USHORT	usLenCh
)
{
	BOOLEAN		bRet = FALSE;

	CKrnlStr	StrTmp;


	__try
	{
		if (!pChStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (!StrTmp.Set(pChStr, usLenCh))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"StrTmp.Set failed. (%hs)",
				pChStr);

			__leave;
		}

		bRet = Append(&StrTmp);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Append(
__in PWCHAR pWchStr,
__in USHORT	usLenCh
)
{
	BOOLEAN		bRet = FALSE;

	USHORT		usLenBPre = 0;
	USHORT		usLenBPost = 0;
	USHORT		usLenBNew = 0;

	CKrnlStr	StrTemp;


	__try
	{
		if (!pWchStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"NULL == m_Str.Buffer");
			__leave;
		}

		usLenBPre = m_Str.Length;
		usLenBPost = usLenCh * sizeof(WCHAR);
		usLenBNew = usLenBPre + usLenBPost;

		if (usLenBNew >= m_Str.MaximumLength)
		{
			if (!StrTemp.Set(&m_Str))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"StrTmp.Set failed. (%wZ)",
					&m_Str);

				__leave;
			}

			if (!Alloc(usLenBNew / sizeof(WCHAR) + 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}

			m_Str.Length = usLenBPre;
			RtlCopyMemory(m_Str.Buffer, StrTemp.GetString(), m_Str.Length);
		}

		m_Str.Length = usLenBNew;
		RtlCopyMemory(m_Str.Buffer + usLenBPre / sizeof(WCHAR), pWchStr, usLenBPost);

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Append(
__in PUNICODE_STRING pUnicodeStr
)
{
	BOOLEAN	 bRet = FALSE;


	__try
	{
		if (!pUnicodeStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		bRet = Append(pUnicodeStr->Buffer, pUnicodeStr->Length / sizeof(WCHAR));
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Shorten(
__in USHORT usLenCh
)
{
	BOOLEAN bRet = FALSE;

	USHORT	usLenB = 0;


	__try
	{
		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"NULL == m_Str.Buffer");
			__leave;
		}

		usLenB = usLenCh * sizeof(WCHAR);

		if (usLenB > m_Str.Length)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ulLen > m_Str.ulLen. you are lengthen");
			__leave;
		}

		if (usLenB < m_Str.Length)
		{
			RtlZeroMemory(m_Str.Buffer + usLenCh, m_Str.Length - usLenB);
			m_Str.Length = usLenB;
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
CKrnlStr::Lengthen(
__in USHORT usLenCh
)
{
	BOOLEAN		bRet = FALSE;

	USHORT		usLenB = 0;

	CKrnlStr	StrTemp;


	__try
	{
		usLenB = usLenCh * sizeof(WCHAR);

		if (usLenB < m_Str.Length)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"usLenB < m_Str.Length. you are shorten");
			__leave;
		}

		if (usLenB < m_Str.MaximumLength)
			m_Str.Length = usLenB;
		else if (usLenB >= m_Str.MaximumLength)
		{
			if (m_Str.Length)
			{
				if (!StrTemp.Set(&m_Str))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"StrTemp.Set failed. (%wZ)",
						&m_Str);

					__leave;
				}
			}

			if (!Alloc(usLenCh + 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}

			if (StrTemp.GetLenB())
				RtlCopyMemory(m_Str.Buffer, StrTemp.GetString(), StrTemp.GetLenB());

			m_Str.Length = usLenB;
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
CKrnlStr::Clean()
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"NULL == m_Str.Buffer");
			__leave;
		}

		RtlZeroMemory(m_Str.Buffer, m_Str.MaximumLength);
		m_Str.Length = 0;

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
CKrnlStr::Init()
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!m_Str.Buffer)
		{
			if (!Alloc(MAX_PATH))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}
		}
		else
		{
			if (!Clean())
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Clean failed");
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
CKrnlStr::WcharStrEqual(
__in PWCHAR		pWchPosition1,
__in USHORT		usLenCh1,
__in PWCHAR		pWchPosition2,
__in USHORT		usLenCh2,
__in USHORT		usLenChCmp,
__in BOOLEAN	bIgnoreCase
)
{
	BOOLEAN	bRet = FALSE;

	USHORT	i = 0;
	USHORT	usLenChTemp = 0;
	PWCHAR	pWchTemp = NULL;
	USHORT	usDifference = L'a' - L'A';


	__try
	{
		if (!pWchPosition1 || !pWchPosition2 || !usLenChCmp)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. (%p) (%p) (%d)",
				pWchPosition1, pWchPosition2, usLenChCmp);

			__leave;
		}

		if (usLenChCmp > usLenCh1 && usLenChCmp > usLenCh2)
			__leave;

		if (usLenCh1 > usLenCh2)
		{
			pWchTemp = pWchPosition1;
			pWchPosition1 = pWchPosition2;
			pWchPosition2 = pWchTemp;

			usLenChTemp = usLenCh1;
			usLenCh1 = usLenCh2;
			usLenCh2 = usLenChTemp;
		}

		if (usLenChCmp > usLenCh1 && usLenChCmp <= usLenCh2)
			__leave;

		for (; i < usLenChCmp; i++, pWchPosition1++, pWchPosition2++)
		{
			if (*pWchPosition1 != *pWchPosition2)
			{
				if ((L'A' <= *pWchPosition1) && (L'Z' >= *pWchPosition1))
				{
					if (bIgnoreCase)
					{
						if ((*pWchPosition1 + usDifference) == *pWchPosition2)
							continue;
					}
				}
				else if ((L'a' <= *pWchPosition1) && (L'z' >= *pWchPosition1))
				{
					if (bIgnoreCase)
					{
						if ((*pWchPosition1 - usDifference) == *pWchPosition2)
							continue;
					}
				}

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

PWCHAR
CKrnlStr::SearchCharacter(
__in WCHAR	wch,
__in PWCHAR	pWchBegin,
__in PWCHAR	pWchEnd
)
{
	PWCHAR	pRet = NULL;

	PWCHAR	pWchPositionCur = NULL;


	__try
	{
		if (!pWchBegin || !pWchEnd)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. (%p) (%p)",
				pWchBegin, pWchEnd);

			__leave;
		}

		if (pWchBegin < pWchEnd)
		{
			for (pWchPositionCur = pWchBegin; pWchPositionCur <= pWchEnd; pWchPositionCur++)
			{
				if (wch == *pWchPositionCur)
				{
					pRet = pWchPositionCur;
					break;
				}
			}
		}
		else
		{
			for (pWchPositionCur = pWchBegin; pWchPositionCur >= pWchEnd; pWchPositionCur--)
			{
				if (wch == *pWchPositionCur)
				{
					pRet = pWchPositionCur;
					break;
				}
			}
		}
	}
	__finally
	{
		;
	}

	return pRet;
}

PWCHAR
CKrnlStr::Search(
__in PWCHAR		pDes,
__in BOOLEAN	bIgnoreCase
)
{
	PWCHAR		pRet = NULL;

	ULONG		ulIndex = 0;

	CKrnlStr	strTemp;


	__try
	{
		if (!pDes)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (m_Str.Length / sizeof(WCHAR) < wcslen(pDes))
			__leave;

		for (; ulIndex < m_Str.Length / sizeof(WCHAR); ulIndex++)
		{
			if (wcslen(m_Str.Buffer + ulIndex) < wcslen(pDes))
				__leave;

			if (!strTemp.Set(m_Str.Buffer + ulIndex, (USHORT)wcslen(pDes)))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Str1.Set failed. (%lS)",
					m_Str.Buffer + ulIndex);

				__leave;
			}

			if (strTemp.Equal(pDes, (USHORT)wcslen(pDes), bIgnoreCase))
			{
				pRet = m_Str.Buffer + ulIndex;
				__leave;
			}
		}
	}
	__finally
	{
		;
	}

	return pRet;
}

ULONG
CKrnlStr::GetMaxLenCh()
{
	return m_Str.MaximumLength / sizeof(WCHAR);
}
