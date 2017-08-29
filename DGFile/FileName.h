
/*++

CopyRight (C) 2017 - 2017 ���ݻ�;������޹�˾

ģ������FileName.h

ժ  Ҫ��

��  �ߣ�����

--*/

#pragma once

class CFileName
{
public:
	BOOLEAN
		GetVolDevNameByQueryObj(
			__in	CKrnlStr * pSymName,
			__out	CKrnlStr * pDevName
		);

	BOOLEAN
		AppToDev(
			__in	CKrnlStr * pAppName,
			__inout CKrnlStr * pDevName
		);

	BOOLEAN
		AppToSym(
			__in CKrnlStr * pAppName,
			__in CKrnlStr * pSymName
		);

	BOOLEAN
		SymToDev(
			__in	CKrnlStr * pSymName,
			__inout CKrnlStr * pDevName
		);

	static
		BOOLEAN
		GetParentPath(
			__in	CKrnlStr * pPath,
			__out	CKrnlStr * pParentPath
		);


	static
		BOOLEAN
		SystemRootToDev(
			__in	CKrnlStr* pFileName,
			__inout CKrnlStr* pFileNameDev
		);


	/*
	* ����˵����
	*		ƴ���ļ���
	*
	* ������
	*		pDirPath		�ļ������ļ���
	*		pFileName		�ļ���
	*		pFilePath		�ļ�ȫ·��
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	static
		BOOLEAN
		SpliceFilePath(
			__in	CKrnlStr*	pDirPath,
			__in	CKrnlStr*	pFileName,
			__inout	CKrnlStr*	pFilePath
		);

private:



	static
		BOOLEAN
		ConvertByZwQuerySymbolicLinkObject(
			__in	CKrnlStr* pFileName,
			__inout CKrnlStr* pNewFileName
		);
};
