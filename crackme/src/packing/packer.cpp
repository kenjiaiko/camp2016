#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>
#include <commdlg.h>

#include "resource.h"

#include "des.h"
#pragma comment(lib, "des.lib")

HINSTANCE g_hInstance;

BOOL CALLBACK Dlg_Proc(HWND, UINT, WPARAM, LPARAM);

int idc_fileopen(HWND hWnd);
int id_ok(HWND hWnd);
void addText(HWND, TCHAR *, DWORD);

BOOL FileDialog(HWND, TCHAR *, DWORD, TCHAR *, TCHAR *);

LPVOID PeFileRead(LPCTSTR, DWORD *);
VOID PeFileReadFree(LPVOID, DWORD);
DWORD PeFileWrite(LPCTSTR, LPVOID, DWORD);

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

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
	g_hInstance = hInstance;
	HWND hWnd = FindWindow( _T("#32770"), _T("LCryper"));
	if(IsWindow(hWnd))
		return 1;
	DialogBox(hInstance, 
		MAKEINTRESOURCE(IDD_DIALOG), NULL, Dlg_Proc);
	return 0;
}

BOOL CALLBACK Dlg_Proc(HWND   hWnd,
					   UINT   uMsg,
					   WPARAM wParam,
					   LPARAM lParam)
{
	static HWND ClipWnd = NULL;

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_FILEOPEN:
			idc_fileopen(hWnd);
			break;
		case IDOK:
			id_ok(hWnd);
			break;
		case IDCANCEL:
			EndDialog(hWnd, NULL);
			break;
		}
	}
    return 0;
}


int idc_fileopen(HWND hWnd)
{
	TCHAR szFilePath[1024];
	BOOL bret = FileDialog(hWnd, szFilePath, sizeof(szFilePath),
		_T("Open File Dialog"), _T("*.exe\0*.*\0\0"));
	if( ! bret)
		return 1;
	Edit_SetText(GetDlgItem(hWnd, IDC_FILE), szFilePath);
	return 0;
}

int id_ok(HWND hWnd)
{
	Edit_SetText(GetDlgItem(hWnd, IDC_STATUS), _T("Status:\r\n"));

	addText(hWnd, _T("Open Resource ... "), IDC_STATUS);

	// カスタムリソース読み込み
	HRSRC hResource = FindResource(g_hInstance, 
		MAKEINTRESOURCE(IDR_EXE_BIN), _T("EXE_BIN"));
	if(hResource == NULL)
		return PopMsg(_T("Error: FindResource"));
	DWORD dwResSize = SizeofResource(g_hInstance, hResource);
	if(dwResSize == 0)
		return PopMsg(_T("Error: SizeofResource"));
	HGLOBAL hResourceLoaded = LoadResource(g_hInstance, hResource);
	if(hResourceLoaded == NULL)
		return PopMsg(_T("Error: LoadResource"));
	LPVOID lpModelFile = (LPVOID)LockResource(hResourceLoaded); 
	if(lpModelFile == NULL)
		return PopMsg(_T("Error: LockResource"));

	addText(hWnd, _T(
		"OK\r\n"
		"Reading EXE file ... "), IDC_STATUS);

	// ファイル読み込み
	TCHAR szFilePath[1024];
	Edit_GetText(GetDlgItem(hWnd, IDC_FILE), 
		szFilePath, sizeof(szFilePath));
	DWORD dwSize;
	LPVOID lpDataFile = PeFileRead(szFilePath, &dwSize);
	if(dwSize == 0)
		return PopMsg(_T("Error: PeFileRead"));

	addText(hWnd, _T(
		"OK\r\n"
		"Writing BACK UP file ... "), IDC_STATUS);

	// バックアップファイル書き込み
	TCHAR szBackupFile[1024];
	wsprintf(szBackupFile, _T("%s.back"), szFilePath);
	PeFileWrite(szBackupFile, lpDataFile, dwSize);

	addText(hWnd, _T(
		"OK\r\n"
		"Crypting EXE file ... "), IDC_STATUS);

	// 暗号化
	BYTE *szData = (BYTE *)lpDataFile;
	ReverseData(_T("StarWars"), szData, dwSize, TRUE);
	
	// ビット反転
	for(DWORD i=0; i < dwSize; i++)
		szData[i] =~ szData[i];

	addText(hWnd, _T(
		"OK\r\n"
		"Writing EXE file ... "), IDC_STATUS);

	// モデルファイル書き込み
	PeFileWrite(szFilePath, lpModelFile, dwResSize);
	HANDLE hUpResource = BeginUpdateResource(szFilePath, TRUE);
	if (NULL == hUpResource)
		return PopMsg(_T("Error: BeginUpdateResource"));
    BOOL bRet = UpdateResource(hUpResource, _T("EXE_BIN"), 
		MAKEINTRESOURCE(101), MAKELANGID(LANG_NEUTRAL, 
		SUBLANG_DEFAULT), (LPVOID)lpDataFile, dwSize);
	if( ! bRet)
		return PopMsg(_T("Error: UpdateResource"));
	EndUpdateResource(hUpResource, FALSE);

	addText(hWnd, _T(
		"OK\r\n"
		"Complete!"), IDC_STATUS);

	// メモリ開放
	PeFileReadFree(lpDataFile, dwSize);
	return 0;
}

void addText(HWND hWnd, 
			 TCHAR *szText, 
			 DWORD ID)
{
	DWORD dwLen = Edit_GetTextLength(GetDlgItem(hWnd, ID));
	TCHAR *szOutput = new TCHAR[dwLen + lstrlen(szText) + 256];
	Edit_GetText(GetDlgItem(hWnd, ID), szOutput, dwLen + 1);
	lstrcat(szOutput, szText);
	Edit_SetText(GetDlgItem(hWnd, ID), szOutput);
	delete [] szOutput;
}

BOOL FileDialog(HWND  hWnd,
				TCHAR *szPath,
				DWORD dwSize,
				TCHAR *szTitle,
				TCHAR *szFilter)
{
	// ファイル名を入れる配列
	TCHAR szName[512];
	DWORD dwNameLen = sizeof(szName);

	// ファイルダイアログオープン
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));   
    ofn.lStructSize    = sizeof(OPENFILENAME);
    ofn.hwndOwner      = hWnd;
    ofn.lpstrFile      = szPath;
    ofn.nMaxFile       = dwSize;
    ofn.lpstrFileTitle = szName;
    ofn.nMaxFileTitle  = dwNameLen;
    ofn.lpstrFilter    = szFilter;
    ofn.lpstrTitle     = szTitle;
    ofn.Flags          = OFN_FILEMUSTEXIST;
	
	if(!GetSaveFileName(&ofn))
		return FALSE;
	return TRUE;
}

LPVOID PeFileRead(LPCTSTR lpFile, DWORD *pdwSize)
{
	HANDLE hFile;
	LPVOID lpBuffer;

	try{
	
		hFile = CreateFile(
			lpFile, GENERIC_READ, 0, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			throw 1;

		*pdwSize = GetFileSize(hFile, NULL);
		if(*pdwSize == 0xFFFFFFFF)
			throw 2;

		lpBuffer = VirtualAlloc(
			NULL, *pdwSize, MEM_COMMIT, PAGE_READWRITE);
		if(lpBuffer == NULL)
			throw 3;

		DWORD dwReadSize;
		if(!ReadFile(hFile, lpBuffer, *pdwSize, &dwReadSize, NULL))
			throw 4;

	}catch(int err){

		switch(err)
		{
		case 4:
			VirtualFree(lpBuffer, *pdwSize, MEM_DECOMMIT);
		case 1: case 2: case 3:
			CloseHandle(hFile);
		}
		return 0;
	}

	CloseHandle(hFile);
	return lpBuffer;
}

VOID PeFileReadFree(LPVOID lpBuffer, DWORD dwSize)
{
	VirtualFree(lpBuffer, dwSize, MEM_DECOMMIT);
}

DWORD PeFileWrite(LPCTSTR lpFile, LPVOID lpBuffer, DWORD dwSize)
{
	HANDLE hFile;

	try{
	
		hFile = CreateFile(
			lpFile, GENERIC_WRITE, 0, NULL, 
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			throw 1;
		
		DWORD dwWriteSize;
		if(!WriteFile(hFile, lpBuffer, dwSize, &dwWriteSize, NULL))
			throw 2;

	}catch(int err){

		switch(err)
		{
		case 2:
			CloseHandle(hFile);
		}
		return 0;
	}

	CloseHandle(hFile);
	return dwSize;
}
