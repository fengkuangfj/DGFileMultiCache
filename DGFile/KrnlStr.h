
/*++

CopyRight (C) 2017 - 2017 ���ݻ�;������޹�˾

ģ������KrnlStr.h

ժ  Ҫ��

��  �ߣ�����

--*/

#pragma once

#define KRNLSTR_TAG 'TSNK'

/*
* UNICODE_STRING��װ��
*
* ��ע��
*
*/
class CKrnlStr
{
public:
	CKrnlStr();

	~CKrnlStr();

	/*
	* ����˵����
	*		��ʼ��һ���ַ���
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		TURE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Init();

	/*
	* ����˵����
	*		����m_Str.Buffer������
	*
	* ������
	*		pStr	Ҫ���õ���������
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Set(
			__in PWCHAR pWchStr,
			__in USHORT usLenCh
		);

	/*
	* ����˵����
	*		����m_Str.Buffer������
	*
	* ������
	*		pStr	Ҫ���õ���������
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Set(
			__in PCHAR	pChStr,
			__in USHORT usLenCh
		);

	/*
	* ����˵����
	*		����m_Str������
	*
	* ������
	*		pUnicodeStr	Ҫ���õ���������
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Set(
			__in PUNICODE_STRING pUnicodeStr
		);

	/*
	* ����˵����
	*		����һ�ݸ���
	*
	* ������
	*		pStr
	*
	* ����ֵ��
	*		��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Set(
			__in CKrnlStr * pCKrnlStr
		);

	/*
	* ����˵����
	*		���m_Str.Length�ַ�����
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		m_Str.Length�ַ�����
	*
	* ��ע��
	*		��
	*/
	USHORT
		GetLenCh();

	/*
	* ����˵����
	*		���m_Str.Length�ֽڳ���
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		m_Str.Length�ֽڳ���
	*
	* ��ע��
	*		��
	*/
	USHORT
		GetLenB();

	/*
	* ����˵����
	*		���m_Str.Buffer
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		m_Str.Buffer
	*
	* ��ע��
	*		��
	*/
	PWCHAR
		GetString();

	/*
	* ����˵����
	*		���m_Str
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		m_Str
	*
	* ��ע��
	*		��
	*/
	PUNICODE_STRING
		Get();

	/*
	* ����˵����
	*		����ַ��������ݣ����ͷ�Buffer
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
		Clean();

	/*
	* ����˵����
	*		�ж��Ƿ����
	*
	* ������
	*		pStr
	*		IsIgnoreCase �Ƿ���Դ�Сд
	*
	* ����ֵ��
	*		TRUE	���
	*		FALSE	�����
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Equal(
			__in CKrnlStr *	pCKrnlStr,
			__in BOOLEAN	bIgnoreCase
		);

	/*
	* ����˵����
	*		�ж��Ƿ����
	*
	* ������
	*		pWstr
	*		IsIgnoreCase �Ƿ���Դ�Сд
	*
	* ����ֵ��
	*		TRUE	���
	*		FALSE	�����
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Equal(
			__in PWCHAR		pWchStr,
			__in USHORT		usLenCh,
			__in BOOLEAN	bIgnoreCase
		);

	/*
	* ����˵����
	*		�ж��Ƿ����
	*
	* ������
	*		pUstr
	*		IsIgnoreCase �Ƿ���Դ�Сд
	*
	* ����ֵ��
	*		TRUE	���
	*		FALSE	�����
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Equal(
			__in PCHAR		pChStr,
			__in USHORT		usLenCh,
			__in BOOLEAN	bIgnoreCase
		);

	/*
	* ����˵����
	*		�ж��Ƿ����
	*
	* ������
	*		pUnicodeStr
	*		IsIgnoreCase �Ƿ���Դ�Сд
	*
	* ����ֵ��
	*		TRUE	���
	*		FALSE	�����
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Equal(
			__in PUNICODE_STRING	pUnicodeStr,
			__in BOOLEAN			bIgnoreCase
		);

	/*
	* ����˵����
	*		����
	*
	* ������
	*		pUstr
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Append(
			__in PCHAR	pChStr,
			__in USHORT	usLenCh
		);

	/*
	* ����˵����
	*		����
	*
	* ������
	*		pWstr
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Append(
			__in PWCHAR pWchStr,
			__in USHORT	usLenCh
		);

	/*
	* ����˵����
	*		����
	*
	* ������
	*		pUnicodeStr
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Append(
			__in PUNICODE_STRING pUnicodeStr
		);

	/*
	* ����˵����
	*		����
	*
	* ������
	*		pStr
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Append(
			__in CKrnlStr * pCKrnlStr
		);

	/*
	* ����˵����
	*		����ַ�����ʼλ�õ�ָ�������ַ���
	*
	* ������
	*		ulLen	Ҫ��õĳ���(�ֽ���)
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��ʼ���ж�len���п��ܾ���Ҫ��ó���0
	*/
	BOOLEAN
		Shorten(
			__in USHORT usLenCh
		);

	/*
	* ����˵����
	*		��չ�ַ�������
	*
	* ������
	*		ulLen	Ҫ��չ���ĳ���(�ֽ���)
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��ʼ���ж�len���п��ܾ���Ҫ��չ������0
	*/
	BOOLEAN
		Lengthen(
			__in USHORT usLenCh
		);

	/*
	* ����˵����
	*		ת��д
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		&m_Str	תΪ��д��UNICODE_STRING�ַ���
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		ToUpper();

	/*
	* ����˵����
	*		��һ���ַ����в���һ���ַ�
	*
	* ������
	*		wch			Ҫ���ҵ��ַ�
	*		pWchBegin	���ҿ�ʼλ��
	*		pWchEnd		���ҽ���λ��
	*
	* ����ֵ��
	*		�����ַ�����λ��
	*
	* ��ע��
	*		��
	*/
	PWCHAR
		SearchCharacter(
			__in WCHAR	wch,
			__in PWCHAR	pWchBegin,
			__in PWCHAR	pWchEnd
		);

	PWCHAR
		Search(
			__in PWCHAR		pDes,
			__in BOOLEAN	bIgnoreCase
		);

	ULONG
		GetMaxLenCh();

private:
	UNICODE_STRING m_Str;

	/*
	* ����˵����
	*		����
	*
	* ������
	*		ulLen		Ҫ����ĳ���(�ַ���)
	*
	* ����ֵ��
	*		TRUE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		Alloc(
			__in USHORT usLenCh
		);

	/*
	* ����˵����
	*		�ͷ�
	*
	* ������
	*		��
	*
	* ����ֵ��
	*		��
	*
	* ��ע��
	*		��
	*/
	VOID
		Free();

	/*
	* ����˵����
	*		�ȽϿ��ַ����Ƿ����
	*
	* ������
	*		pwchPosition1	��һ�ַ���ָ��
	*		pwchPosition2	�ڶ����ַ���ָ��
	*		ulLen			Ҫ�Ƚϵĳ���
	*		bIgnoreCase		�Ƿ���Դ�Сд
	*
	* ����ֵ��
	*		TURE	�ɹ�
	*		FALSE	ʧ��
	*
	* ��ע��
	*		��
	*/
	BOOLEAN
		WcharStrEqual(
			__in PWCHAR		pWchPosition1,
			__in USHORT		usLenCh1,
			__in PWCHAR		pWchPosition2,
			__in USHORT		usLenCh2,
			__in USHORT		usLenChCmp,
			__in BOOLEAN	bIgnoreCase
		);
};
