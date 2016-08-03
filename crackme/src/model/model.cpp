#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>

#include "resource.h"

#include "LoaderLib.h"
#pragma comment(lib, "LoaderLib.lib")

#include "des.h"
#pragma comment(lib, "des.lib")

int PopMsg(LPCTSTR pszFormat, ...)
{
	va_list argList;
	va_start(argList, pszFormat);
	TCHAR sz[1024];
	_vstprintf(sz, pszFormat, argList);
	va_end(argList);
	MessageBox(GetActiveWindow(), sz, _T("Message"), MB_OK);
	return -1;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	// �J�X�^�����\�[�X�ǂݍ���
	HRSRC hResource = FindResource(hInstance, 
		MAKEINTRESOURCE(IDR_EXE_BIN), _T("EXE_BIN"));
	if(hResource == NULL)
		return PopMsg(_T("Error: FindResource"));
	DWORD dwResSize = SizeofResource(hInstance, hResource);
	if(dwResSize == 0)
		return PopMsg(_T("Error: SizeofResource"));
	HGLOBAL hResourceLoaded = LoadResource(hInstance, hResource);
	if(hResourceLoaded == NULL)
		return PopMsg(_T("Error: LoadResource"));
	LPVOID lpBuffer = (LPVOID)LockResource(hResourceLoaded); 
	if(lpBuffer == NULL)
		return PopMsg(_T("Error: LockResource"));

	// �������m��
	LPVOID lpFileData = VirtualAlloc(NULL, 
		dwResSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(lpFileData == NULL)
		return PopMsg(_T("Error: VirtualAlloc"));

	// �f�[�^�R�s�[�i�r�b�g���]�j
	BYTE *szRes = (BYTE *)lpBuffer;
	BYTE *szMem = (BYTE *)lpFileData;
	for(DWORD i=0; i < dwResSize; i++)
		szMem[i] = ~szRes[i];

	// ������
	ReverseData(_T("StarWars"), szMem, dwResSize, FALSE);

	TCHAR szMyFileName[1024];
	GetModuleFileName(NULL, szMyFileName, sizeof(szMyFileName));

	// �v���Z�X�N��
	int ret = CreateExeProcess(
        lpFileData, dwResSize, szMyFileName);
	if(ret == -1)
		return PopMsg(_T("Error: CreateExeProcess"));

	// �������J��
	VirtualFree(lpFileData, dwResSize, MEM_DECOMMIT);
	return 0;
}
