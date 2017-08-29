
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：FileName.h

摘  要：

作  者：岳翔

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
