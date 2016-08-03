// MessageBox.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "MessageBox.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	MessageBox(NULL, L"Hello World", L"MessageBox", MB_OK);
	return 0;
}
