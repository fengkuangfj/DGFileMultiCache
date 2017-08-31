
/*++

CopyRight (C) 2017 - 2017 ���ݻ�;������޹�˾

ģ������Communication.h

ժ  Ҫ��

��  �ߣ�����

--*/

#pragma once

#define COMMUNICATION_TAG 'MMOC' // COMM

typedef struct _COMM_INFO
{
	PFLT_PORT		pSeverPort;		// �˿�
	PFLT_PORT		pClientPort;	// �˿�
} COMM_INFO, *PCOMM_INFO, *LPCOMM_INFO;

class CCommunication
{
public:
	CCommunication();

	~CCommunication();

	/*
	* ����˵����
	*		��ʼ��ͨѶ
	*
	* ������
	*		pFlt
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*
	*/
	BOOLEAN
		Init(
		__in PFLT_FILTER pFlt
		);

	/*
	* ����˵����
	*		ж��ͨѶģ��
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		��
	*
	* ��ע��
	*
	*/
	VOID
		Unload();

private:
	static COMM_INFO	ms_CommInfo;

	/*
	* ����˵����
	*		ͨ�����ӻص�����
	*
	* ������
	*		ClientPort
	*		ServerPortCookie
	*		ConnectionContext
	*		SizeOfContext
	*		ConnectionPortCookie		Pointer to information that uniquely identifies this client port.
	*
	* ����ֵ��
	*		Status
	*
	* ��ע��
	*
	*/
	static
		NTSTATUS
		CommKmConnectNotify(
		__in			PFLT_PORT	pClientPort,
		__in_opt		PVOID		lpServerPortCookie,
		__in_bcount_opt(ulSizeOfContext) PVOID lpConnectionContext,
		__in			ULONG		ulSizeOfContext,
		__deref_out_opt PVOID*		pConnectionPortCookie
		);

	/*
	* ����˵����
	*		ͨ�ŶϿ��ص�����
	*
	* ������
	*		ConnectionCookie
	*
	* ����ֵ��
	*		��
	*
	* ��ע��
	*
	*/
	static
		VOID
		CommKmDisconnectNotify(
		__in_opt PVOID lpConnectionCookie
		);

	/*
	* ����˵����
	*		R0����R3��������ص�����
	*
	* ������
	*		PortCookie
	*		InputBuffer
	*		InputBufferLength
	*		OutputBuffer
	*		OutputBufferLength
	*		ReturnOutputBufferLength
	*
	* ����ֵ��
	*		Status
	*
	* ��ע��
	*
	*/
	static
		NTSTATUS
		CommKmMessageNotify(
		__in_opt	PVOID	lpPortCookie,
		__in_bcount_opt(ulInputBufferLength) PVOID lpInputBuffer,
		__in		ULONG	ulInputBufferLength,
		__out_bcount_part_opt(ulOutputBufferLength, *pulReturnOutputBufferLength) PVOID lpOutputBuffer,
		__in		ULONG	ulOutputBufferLength,
		__out		PULONG	pulReturnOutputBufferLength
		);
};
