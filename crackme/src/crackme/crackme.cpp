#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdlib.h>

#include "resource.h"

BOOL CALLBACK Dlg_Proc(HWND   hWnd,
					   UINT   uMsg,
					   WPARAM wParam,
					   LPARAM lParam);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
	HWND hWnd = FindWindow( _T("#32770"), _T("crackme"));
	if(IsWindow(hWnd))
		return 1;
	DialogBox(hInstance, 
		MAKEINTRESOURCE(IDD_DIALOG), NULL, Dlg_Proc);
	return 0;
}

void id_ok(HWND hWnd)
{
	TCHAR szPass[1024];
	Edit_GetText(GetDlgItem(hWnd, IDC_EDIT), 
		szPass, sizeof(szPass));
	if(szPass[0] == _T('\0'))
		return;
	
	if(atoi(szPass) != 2006020515)
		return;

	TCHAR szOkString[1024];
	wsprintf(szOkString, 
		_T("ZIP file passwd is \"%s00yen\"."), szPass);
	MessageBox(GetActiveWindow(), 
		szOkString, _T("message"), MB_OK);
	SetWindowText(GetDlgItem(hWnd, IDC_STRING), szOkString);
	Edit_Enable(GetDlgItem(hWnd, IDC_EDIT), FALSE);
	Button_Enable(GetDlgItem(hWnd, IDOK), FALSE);
	return;
}

BOOL CALLBACK Dlg_Proc(HWND   hWnd,
					   UINT   uMsg,
					   WPARAM wParam,
					   LPARAM lParam)
{
    switch (uMsg) 
    {
    case WM_INITDIALOG:
        break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
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

