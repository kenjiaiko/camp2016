
#include <Windows.h>
#include <tchar.h>
#include <shlobj.h>

int cpy(void)
{
	// ���t�@�C���p�X�擾
	TCHAR szThis[2048];
	GetModuleFileName(NULL, szThis, sizeof(szThis));
	// �X�^�[�g�A�b�v�t�@�C���p�X�擾
	TCHAR szStartup[2048];
	SHGetFolderPath(NULL, CSIDL_STARTUP, 
		NULL, SHGFP_TYPE_CURRENT, szStartup);
	lstrcat(szStartup, _T("\\wsample01b.exe"));
	// �X�^�[�g�A�b�v�֎������R�s�[
	CopyFile(szThis, szStartup, FALSE);
	return 0;
}

int APIENTRY _tWinMain(
	HINSTANCE hInstance, 
	HINSTANCE hPrevInstance, 
	LPTSTR    lpCmdLine, 
	int       nCmdShow)
{
	cpy();
	MessageBox(GetActiveWindow(), 
		_T("Copied!"), _T("MESSAGE"), MB_OK);
	return 0;
}