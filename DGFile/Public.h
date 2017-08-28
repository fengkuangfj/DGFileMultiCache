
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：Public.h

摘  要：

作  者：岳翔

--*/

#pragma once

#define	PUBLIC_TAG 'LBUP'

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

typedef enum _LOG_PRINTF_LEVEL
{
	LOG_PRINTF_LEVEL_INFO,
	LOG_PRINTF_LEVEL_WARNING,
	LOG_PRINTF_LEVEL_ERROR,
	LOG_PRINTF_LEVEL_ILLEGALITY
} LOG_PRINTF_LEVEL, *PLOG_PRINTF_LEVEL, *LPLOG_PRINTF_LEVEL;

typedef enum _LOG_RECORED_LEVEL
{
	LOG_RECORED_LEVEL_NEED,
	LOG_RECORED_LEVEL_NEEDNOT
} LOG_RECORED_LEVEL, *PLOG_RECORED_LEVEL, *LPLOG_RECORED_LEVEL;

#define KdPrintKrnl(PrintfLevel, RecoredLevel, FMT, ...) PrintKrnl(PrintfLevel, RecoredLevel, __FUNCTION__, __LINE__, FMT, __VA_ARGS__)

extern "C"
UCHAR*
PsGetProcessImageFileName(
__in PEPROCESS pEprocess
);

VOID
PrintKrnl(
__in							LOG_PRINTF_LEVEL	PrintfLevel,
__in							LOG_RECORED_LEVEL	RecoredLevel,
__in							PCHAR				pFuncName,
__in							ULONG				ulLine,
__in __drv_formatString(printf)	PWCHAR				Fmt,
...
);

PVOID
__cdecl operator
new(
size_t	size,
ULONG	ulPoolTag
);

VOID
__cdecl operator
delete(
PVOID lpPointer
);

VOID
__cdecl operator
delete[](
PVOID lpPointer
);

VOID
Sleep(
__in LONGLONG llTimeMilliseconds
);
