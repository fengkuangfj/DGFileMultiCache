
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：Communication.h

摘  要：

作  者：岳翔

--*/

#pragma once

#define COMMUNICATION_TAG 'MMOC' // COMM

typedef struct _COMM_INFO
{
	PFLT_PORT		pSeverPort;		// 端口
	PFLT_PORT		pClientPort;	// 端口
} COMM_INFO, *PCOMM_INFO, *LPCOMM_INFO;

class CCommunication
{
public:
	CCommunication();

	~CCommunication();

	/*
	* 函数说明：
	*		初始化通讯
	*
	* 参数：
	*		pFlt
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*
	*/
	BOOLEAN
		Init(
		__in PFLT_FILTER pFlt
		);

	/*
	* 函数说明：
	*		卸载通讯模块
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		无
	*
	* 备注：
	*
	*/
	VOID
		Unload();

private:
	static COMM_INFO	ms_CommInfo;

	/*
	* 函数说明：
	*		通信连接回调函数
	*
	* 参数：
	*		ClientPort
	*		ServerPortCookie
	*		ConnectionContext
	*		SizeOfContext
	*		ConnectionPortCookie		Pointer to information that uniquely identifies this client port.
	*
	* 返回值：
	*		Status
	*
	* 备注：
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
	* 函数说明：
	*		通信断开回调函数
	*
	* 参数：
	*		ConnectionCookie
	*
	* 返回值：
	*		无
	*
	* 备注：
	*
	*/
	static
		VOID
		CommKmDisconnectNotify(
		__in_opt PVOID lpConnectionCookie
		);

	/*
	* 函数说明：
	*		R0处理R3发来请求回调函数
	*
	* 参数：
	*		PortCookie
	*		InputBuffer
	*		InputBufferLength
	*		OutputBuffer
	*		OutputBufferLength
	*		ReturnOutputBufferLength
	*
	* 返回值：
	*		Status
	*
	* 备注：
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
