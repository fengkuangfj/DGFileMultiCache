
/*++

CopyRight (C) 2017 - 2017 ���ݻ�;������޹�˾

ģ������Log.h

ժ  Ҫ��

��  �ߣ�����

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
	* ����˵����
	*		��ʼ��
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Init();

	/*
	* ����˵����
	*		ж��
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Unload();

	/*
	* ����˵����
	*		��ʼ����־�ļ�
	*
	* ������
	*		bReset	�Ƿ�������־�ļ�
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
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
	* ����˵����
	*		�ͷ���־�ļ�
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		ReleaseLogFile();

	/*
	* ����˵����
	*		������־�ļ�����Ŀ¼
	*
	* ������
	*		pLogDir	��־�ļ�����Ŀ¼
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��

	* ��ע��
	*		��
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
	* ����˵����
	*		��ȡ��־
	*
	* ������
	*		pLog	��־
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Pop(
		__out CKrnlStr * pLog
		);

	/*
	* ����˵����
	*		д��־
	*
	* ������
	*		pLog	��־
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Write(
		__in CKrnlStr * pLog
		);

	/*
	* ����˵����
	*		�ж���־�ļ��Ƿ��Ѿ�׼�����
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		TRUE	׼�����
	*		FALSE	��δ׼�����
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		LogFileReady();

	/*
	* ����˵����
	*		��ʼ����־�ļ���
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
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
