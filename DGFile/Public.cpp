
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：Public.cpp

摘  要：

作  者：岳翔

--*/

#include "stdafx.h"
#include "Public.h"

#pragma warning(disable : 4127)
#pragma warning(disable : 4509)

/*
* 分配一个指针
*
* 参数：
*		size		要分配的大小
*		ulPoolTag	要分配的指针的标记
*
* 返回值：
*		无
*
* 备注：
*
*/
PVOID
__cdecl operator
new(
	size_t	size,
	ULONG	ulPoolTag
	)
{
	PVOID lpPointer = NULL;


	__try
	{
		if (!size)
			__leave;

		while (TRUE)
		{
			lpPointer = ExAllocatePoolWithTag(NonPagedPool, size, ulPoolTag);
			if (lpPointer)
			{
				RtlZeroMemory(lpPointer, size);
				break;
			}

			Sleep(10000);
		}
	}
	__finally
	{
		;
	}

	return lpPointer;
}

/*
* 释放一个指针
*
* 参数：
*		pPointer	要释放的指针
*
* 返回值：
*		无
*
* 备注：
*
*/
VOID
__cdecl operator
delete(
	PVOID lpPointer
	)
{
	if (lpPointer)
	{
		ExFreePool(lpPointer);
		lpPointer = NULL;
	}
}

/*
* 释放一个指针
*
* 参数：
*		lpPointer	要释放的指针
*
* 返回值：
*		无
*
* 备注：
*
*/
VOID
__cdecl operator
delete[](
	PVOID lpPointer
	)
{
	if (lpPointer)
	{
		ExFreePool(lpPointer);
		lpPointer = NULL;
	}
}

VOID
PrintKrnl(
	__in							LOG_PRINTF_LEVEL	PrintfLevel,
	__in							LOG_RECORED_LEVEL	RecoredLevel,
	__in							PCHAR				pFuncName,
	__in							ULONG				ulLine,
	__in __drv_formatString(printf)	PWCHAR				Fmt,
	...
)
{
	LARGE_INTEGER	snow = { 0 };
	LARGE_INTEGER	now = { 0 };
	TIME_FIELDS		nowFields = { 0 };
	WCHAR			Time[64] = { 0 };
	PCHAR			pchProcName = NULL;
	WCHAR			wchProcName[20] = { 0 };
	UNICODE_STRING	ustrProcName = { 0 };
	ANSI_STRING		astrProcName = { 0 };
	WCHAR*			pContent = NULL;
	NTSTATUS		ntStatus = STATUS_UNSUCCESSFUL;
	PEPROCESS		pEprocess = NULL;
	ULONG			ulOwningPid = 0;
	ULONG			ulTid = 0;
	WCHAR*			pLogInfo = NULL;
	WCHAR			wchPrintfLevel[10] = { 0 };

	CLog			Log;

	va_list			Args;


	__try
	{
		va_start(Args, Fmt);

		// 获得标准时间
		KeQuerySystemTime(&snow);

		// 转换为当地时间
		ExSystemTimeToLocalTime(&snow, &now);

		// 转换为人们可以理解的时间格式
		RtlTimeToTimeFields(&now, &nowFields);

		// 打印到字符串中
		ntStatus = RtlStringCchPrintfW(
			Time,
			64,
			L"[%04d-%02d-%02d][%02d:%02d:%02d.%03d]",
			nowFields.Year,
			nowFields.Month,
			nowFields.Day,
			nowFields.Hour,
			nowFields.Minute,
			nowFields.Second,
			nowFields.Milliseconds
		);
		if (!NT_SUCCESS(ntStatus))
			RtlCopyMemory(Time, L"[Date Error][Time Error]", wcslen(L"[Date Error][Time Error]") * sizeof(WCHAR));

		ulOwningPid = (ULONG)PsGetCurrentProcessId();

		ulTid = (ULONG)PsGetCurrentThreadId();

		pEprocess = PsGetCurrentProcess();
		if (pEprocess)
			pchProcName = (PCHAR)PsGetProcessImageFileName(pEprocess);

		if (pchProcName)
		{
			astrProcName.Length = (USHORT)strlen(pchProcName) * sizeof(CHAR);
			astrProcName.MaximumLength = astrProcName.Length;
			astrProcName.Buffer = pchProcName;

			do
			{
				ntStatus = RtlAnsiStringToUnicodeString(
					&ustrProcName,
					&astrProcName,
					TRUE
				);
			} while (STATUS_NO_MEMORY == ntStatus);

			if (!NT_SUCCESS(ntStatus))
				RtlCopyMemory(wchProcName, L"Proc Error", wcslen(L"Proc Error") * sizeof(WCHAR));
			else
				RtlCopyMemory(wchProcName, ustrProcName.Buffer, ustrProcName.Length);
		}
		else
			RtlCopyMemory(wchProcName, L"Proc Error", wcslen(L"Proc Error") * sizeof(WCHAR));

		switch (PrintfLevel)
		{
		case LOG_PRINTF_LEVEL_ERROR:
		{
			RtlCopyMemory(wchPrintfLevel, L"ERRO", wcslen(L"ERRO") * sizeof(WCHAR));
			break;
		}
		case LOG_PRINTF_LEVEL_INFO:
		{
			RtlCopyMemory(wchPrintfLevel, L"INFO", wcslen(L"INFO") * sizeof(WCHAR));
			break;
		}
		case LOG_PRINTF_LEVEL_WARNING:
		{
			RtlCopyMemory(wchPrintfLevel, L"WARN", wcslen(L"WARN") * sizeof(WCHAR));
			break;
		}
		case LOG_PRINTF_LEVEL_ILLEGALITY:
		{
			RtlCopyMemory(wchPrintfLevel, L"ILLE", wcslen(L"ILLE") * sizeof(WCHAR));
			break;
		}
		default:
		{
			RtlCopyMemory(wchPrintfLevel, L"UNKN", wcslen(L"UNKN") * sizeof(WCHAR));
			break;
		}
		}

		pContent = (PWCHAR)new(PUBLIC_TAG)CHAR[MAX_PATH * 4 * sizeof(WCHAR)];
		pLogInfo = (PWCHAR)new(PUBLIC_TAG)CHAR[MAX_PATH * 4 * sizeof(WCHAR)];
		ntStatus = RtlStringCchVPrintfW(pContent, MAX_PATH * 4, Fmt, Args);
		if (NT_SUCCESS(ntStatus))
			ntStatus = RtlStringCchPrintfW(
				pLogInfo,
				MAX_PATH * 4,
				L"[%lS]%lS[%05d][%05d][%lS][%hs][%d]%lS",
				wchPrintfLevel,
				Time,
				ulOwningPid,
				ulTid,
				wchProcName,
				pFuncName,
				ulLine,
				pContent
			);
		else
			ntStatus = RtlStringCchPrintfW(
				pLogInfo,
				MAX_PATH * 4,
				L"[%lS]%lS[%05d][%05d][%lS][%hs][%d]%x",
				wchPrintfLevel,
				Time,
				ulOwningPid,
				ulTid,
				wchProcName,
				pFuncName,
				ulLine,
				ntStatus
			);

		if (!NT_SUCCESS(ntStatus))
		{
			ntStatus = RtlStringCchPrintfW(
				pLogInfo,
				MAX_PATH * 4,
				L"RtlStringCchPrintfW failed 1. (%x)",
				ntStatus
			);
			if (!NT_SUCCESS(ntStatus))
			{
				ntStatus = DbgPrintEx(
					DPFLTR_IHVDRIVER_ID,
					DPFLTR_ERROR_LEVEL,
					"RtlStringCchPrintfW failed 2. (%x) \n",
					ntStatus
				);
				if (!NT_SUCCESS(ntStatus))
					DbgPrintEx(
						DPFLTR_IHVDRIVER_ID,
						DPFLTR_ERROR_LEVEL,
						"DbgPrintEx failed 1. (%x) \n",
						ntStatus
					);

				__leave;
			}
		}

		if (RecoredLevel == LOG_RECORED_LEVEL_NEED)
			Log.Insert(pLogInfo, (USHORT)wcslen(pLogInfo));

		ntStatus = DbgPrintEx(
			DPFLTR_IHVDRIVER_ID,
			DPFLTR_ERROR_LEVEL,
			"\n%lS\n",
			pLogInfo
		);
		if (!NT_SUCCESS(ntStatus))
			DbgPrintEx(
				DPFLTR_IHVDRIVER_ID,
				DPFLTR_ERROR_LEVEL,
				"DbgPrintEx failed 2. (%x) \n",
				ntStatus
			);
	}
	__finally
	{
		va_end(Args);

		if (pContent)
		{
			delete[] pContent;
			pContent = NULL;
		}

		if (pLogInfo)
		{
			delete[] pLogInfo;
			pLogInfo = NULL;
		}

		if (ustrProcName.Buffer)
			RtlFreeUnicodeString(&ustrProcName);
	}

	return;
}

VOID
Sleep(
	__in LONGLONG llTimeMilliseconds
)
{
	LARGE_INTEGER liOneHundredNanoseconds = { 0 };

	if (APC_LEVEL >= KeGetCurrentIrql())
	{
		liOneHundredNanoseconds.QuadPart = -1 * llTimeMilliseconds;
		KeDelayExecutionThread(KernelMode, FALSE, &liOneHundredNanoseconds);
	}
	else
	{
		do
		{
			llTimeMilliseconds--;
		} while (0 < llTimeMilliseconds);
	}

	return;
}
