
/*++

CopyRight (C) 2017 - 2017 杭州华途软件有限公司

模块名：File.h

摘  要：

作  者：岳翔

--*/

#pragma once

#define FILE_TAG 'ELIF' // FILE

#define METADATA_FILE_COUNT 12

// 处理对象类型
enum OBJECT_TYPE
{
	OBJECT_TYPE_NULL,
	OBJECT_TYPE_FILE,
	OBJECT_TYPE_DIR,
	OBJECT_TYPE_VOLUME,
	OBJECT_TYPE_UNKNOWN
};

class CFile
{
public:
	static CKrnlStr *					ms_pPassthroughPathEndsWithList;
	static ULONG						ms_ulPassthroughPathEndsWithCount;
	static CKrnlStr *					ms_pPassthroughPathContainsList;
	static ULONG						ms_ulPassthroughPathContainsCount;

	CFile();
	~CFile();

	BOOLEAN
		Init();

	BOOLEAN
		Unload();




	static
		BOOLEAN
		IsShadowCopy(
			__in CKrnlStr * pFileName
		);

	static
		BOOLEAN
		IsExpression(
			__in CKrnlStr* pFileName
		);



	static
		BOOLEAN
		CreateDir(
			__in CKrnlStr * pLogDir
		);


	BOOLEAN
		IsInPassthroughPathEndsWithList(
			__in CKrnlStr * pstrPath
		);

	BOOLEAN
		IsInPassthroughPathContainsList(
			__in CKrnlStr * pstrPath
		);


	static
		BOOLEAN
		IsPageFileSys(
			__in CKrnlStr* pFileName
		);

	static
		BOOLEAN
		IsHiberFilSys(
			__in CKrnlStr* pFileName
		);

	static
		BOOLEAN
		IsSwapFileSys(
			__in CKrnlStr* pFileName
		);

private:
	static CKrnlStr* ms_pSystemRoot;
	static CKrnlStr* ms_pPageFileSys;
	static CKrnlStr* ms_pHiberFilSys;
	static CKrnlStr* ms_pSwapFileSys;



	BOOLEAN
		InitPassthroughPathEndsWithList();

	BOOLEAN
		InitPassthroughPathContainsList();
};
