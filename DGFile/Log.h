
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：Log.h

摘  要：

作  者：岳翔

--*/

#pragma once

#define LOG_TAG	'GOL'

#define	EVERY_TIME_LOG_MAX_COUNT	1
#define MAX_LOG_FILE_SIZE			(9 * 1024 * 1024)

typedef struct _LOG_INFO
{
	CKrnlStr	Log;

	LIST_ENTRY	List;
} LOG_INFO, *PLOG_INFO, *LPLOG_INFO;

class CLog
{
public:
	static PFLT_INSTANCE	ms_pFltInstance;
	static ULONG			ms_ulSectorSize;

	CLog();

	~CLog();

	/*
	* 函数说明：
	*		初始化
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Init();

	/*
	* 函数说明：
	*		卸载
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Unload();

	/*
	* 函数说明：
	*		初始化日志文件
	*
	* 参数：
	*		bReset	是否重置日志文件
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		InitLogFile(
		__in BOOLEAN bReset = FALSE
		);

	BOOLEAN
		Insert(
		__in WCHAR	*	pLog,
		__in USHORT		usLenCh
		);

	VOID
		GetLock();

	VOID
		FreeLock();

	/*
	* 函数说明：
	*		释放日志文件
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		ReleaseLogFile();

	/*
	* 函数说明：
	*		设置日志文件所在目录
	*
	* 参数：
	*		pLogDir	日志文件所在目录
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败

	* 备注：
	*		无
	*/
	BOOLEAN
		SetLogDir(
		__in CKrnlStr * pLogDir
		);

	BOOLEAN
		InitFltInstance();

	BOOLEAN
		InitSectorSize();

private:
	static LIST_ENTRY		ms_ListHead;
	static ERESOURCE		ms_Lock;
	static KSPIN_LOCK		ms_SpLock;
	static KEVENT			ms_WaitEvent;
	static CKrnlStr*		ms_pLogFile;
	static CKrnlStr*		ms_pLogDir;
	static HANDLE			ms_hLogFile;
	static PFILE_OBJECT		ms_pLogFileObj;
	static LARGE_INTEGER	ms_liByteOffset;
	static PETHREAD			ms_pEThread;
	static BOOLEAN			ms_bCanInsertLog;

	static KSTART_ROUTINE	ThreadStart;

	KIRQL					m_Irql;
	LONG					m_LockRef;

	/*
	* 函数说明：
	*		获取日志
	*
	* 参数：
	*		pLog	日志
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Pop(
		__out CKrnlStr * pLog
		);

	/*
	* 函数说明：
	*		写日志
	*
	* 参数：
	*		pLog	日志
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Write(
		__in CKrnlStr * pLog
		);

	/*
	* 函数说明：
	*		判断日志文件是否已经准备完毕
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		TRUE	准备完毕
	*		FALSE	尚未准备完毕
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		LogFileReady();

	/*
	* 函数说明：
	*		初始化日志文件名
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		InitLogFileName();

	BOOLEAN
		InitLogDir(
		__in CKrnlStr *	pLogPath
		);

	BOOLEAN
		IsOldLogFile(
		__in CKrnlStr * pName
		);

	BOOLEAN
		IsSameDate(
		__in CKrnlStr * pLog
		);
};
