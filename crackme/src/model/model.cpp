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
	// カスタムリソース読み込み
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

	// メモリ確保
	LPVOID lpFileData = VirtualAlloc(NULL, 
		dwResSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(lpFileData == NULL)
		return PopMsg(_T("Error: VirtualAlloc"));

	// データコピー（ビット反転）
	BYTE *szRes = (BYTE *)lpBuffer;
	BYTE *szMem = (BYTE *)lpFileData;
	for(DWORD i=0; i < dwResSize; i++)
		szMem[i] = ~szRes[i];

	// 復号化
	ReverseData(_T("StarWars"), szMem, dwResSize, FALSE);

	TCHAR szMyFileName[1024];
	GetModuleFileName(NULL, szMyFileName, sizeof(szMyFileName));

	// プロセス起動
	int ret = CreateExeProcess(
        lpFileData, dwResSize, szMyFileName);
	if(ret == -1)
		return PopMsg(_T("Error: CreateExeProcess"));

	// メモリ開放
	VirtualFree(lpFileData, dwResSize, MEM_DECOMMIT);
	return 0;
}
