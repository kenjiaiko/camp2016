// CopyTempCmd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Windows.h>

int main()
{
	WCHAR path[2048];
	WCHAR temp[2048];
	GetModuleFileName(NULL, path, sizeof(path));
	//MessageBox(NULL, path, L"A", 0);
	GetTempPath(2048, temp);
	lstrcat(temp, L"a.exe");
	//MessageBox(NULL, temp, L"A", 0);
	CopyFile(path, temp, TRUE);
	MessageBox(NULL, L"Hello World", L"Message", 0);
	DeleteFile(temp);
	return 0;
}

