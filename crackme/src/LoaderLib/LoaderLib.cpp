#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "LoaderLib.h"

// MTシグネチャのサイズ
#define SIZE_OF_NT_SIGNATURE    4

// NTシグネチャ
#define NTSIGNATURE(ptr) (  \
    (LPVOID)((PBYTE)(ptr) + \
    ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew))

// PEヘッダオフセット
#define PEFHDROFFSET(ptr) ( \
    (LPVOID)((PBYTE)(ptr) + \
    ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew + \
    SIZE_OF_NT_SIGNATURE))

// オプションヘッダオフセット
#define OPTHDROFFSET(ptr) ( \
    (LPVOID)((PBYTE)(ptr) + \
    ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew + \
    SIZE_OF_NT_SIGNATURE +  \
    sizeof(IMAGE_FILE_HEADER)))

// セクションヘッダオフセット
#define SECHDROFFSET(ptr) ( \
    (LPVOID)((PBYTE)(ptr) + \
    ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew + \
    SIZE_OF_NT_SIGNATURE +  \
    sizeof(IMAGE_FILE_HEADER) + \
    sizeof(IMAGE_OPTIONAL_HEADER)))


// PEヘッダのポインタ構造体
typedef struct {
	PIMAGE_DOS_HEADER       dsh;
	PIMAGE_FILE_HEADER      pfh; 
	PIMAGE_OPTIONAL_HEADER  poh;
	PIMAGE_SECTION_HEADER   psh;
} PE_HEADERS, *PPE_HEADERS;


// 関数の定義
DWORD  GetAlignedSize(DWORD, DWORD);
DWORD  TotalNewImageSize(PPE_HEADERS);
LPVOID LoadPEImage(PPE_HEADERS, LPVOID, DWORD);
VOID   FreePEImage(LPVOID, DWORD);
int    ExecPEImage(PPE_HEADERS, LPVOID, DWORD, PTCHAR);


// -------------------------------------------------------------
// プロセス生成を行うメイン関数
//   引数：pPEFileImage   （ファイルデータのポインタ）
//         dwFileImageSize（ファイルデータのサイズ）
//         szTargetProc   （ターゲットプロセスパス）
// 戻り値：成功時0、失敗時0以外
// -------------------------------------------------------------
int CreateExeProcess(LPVOID pPEFileImage,
					 DWORD dwFileImageSize,
					 PTCHAR szTargetProc)
{
	// それぞれのヘッダポインタ取得
	PE_HEADERS pe;
	pe.dsh = (PIMAGE_DOS_HEADER)pPEFileImage;
	pe.pfh = (PIMAGE_FILE_HEADER)PEFHDROFFSET(pPEFileImage);
	pe.poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(pPEFileImage);
	pe.psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET(pPEFileImage);
	
	// MZ判定
    if(pe.dsh->e_magic != IMAGE_DOS_SIGNATURE)
        return -1;

	// PE判定
    if(*(DWORD *)NTSIGNATURE(pPEFileImage) != IMAGE_NT_SIGNATURE)
        return -1;

	// 実行ファイル判定
	if(pe.poh->Magic != 0x010B)
        return -1;

	// メモリイメージサイズを取得
	DWORD dwMemImageSize = TotalNewImageSize(&pe);
	
	// メモリイメージをロード（メモリ確保）
	LPVOID pMemImage = LoadPEImage(
		&pe, pPEFileImage, dwMemImageSize);
	if(pMemImage == NULL)
		return -1;

	// プロセスを起動
	if(ExecPEImage(&pe, pMemImage, dwMemImageSize, szTargetProc))
		return -1;

	// メモリイメージを開放（メモリ開放）
	FreePEImage(pMemImage, dwMemImageSize);
	return 0;
}

// -------------------------------------------------------------
// dwCurSizeをdwAlignmentの倍数値に変換する関数
//   引数：dwCurSize  （初期値）
//         dwAlignment（倍数単位）
// 戻り値：dwAlignmentの倍数値
// -------------------------------------------------------------
DWORD GetAlignedSize(DWORD dwCurSize, 
					 DWORD dwAlignment)
{
	if(dwCurSize % dwAlignment == 0)
		return dwCurSize;
	return (((dwCurSize / dwAlignment) + 1) * dwAlignment);
}

// -------------------------------------------------------------
// メモリイメージのサイズを計算する関数
//   引数：pe（PEファイルのヘッダポインタ構造体のポインタ）
// 戻り値：メモリイメージのサイズ
// -------------------------------------------------------------
DWORD TotalNewImageSize(PPE_HEADERS pe)
{
	// ヘッダのサイズを代入
	DWORD dwRet = GetAlignedSize(
		pe->poh->SizeOfHeaders, pe->poh->SectionAlignment);

	// セクションごとのサイズを加算
	for(int i=0; i < pe->pfh->NumberOfSections; i++){
		if( ! pe->psh[i].Misc.VirtualSize)
			continue;
		dwRet += GetAlignedSize(
			pe->psh[i].Misc.VirtualSize, pe->poh->SectionAlignment);
	}

	// PEファイルのイメージサイズを返す
	return dwRet;
}

// -------------------------------------------------------------
// ファイルイメージからメモリイメージを生成する関数
//   引数：pe（PEファイルのヘッダポインタ構造体のポインタ）
//         lpFileImage（ファイルイメージのポインタ）
//         dwMemSize  （メモリイメージのサイズ）
// 戻り値：成功時メモリイメージのポインタ、失敗時NULL
// 　備考：プロセスのメモリイメージを格納するためのメモリ領域を
// VirtualAlloc関数にて確保するため、この関数を呼び出したら、そ
// の後FreePEImage関数を呼び出しメモリを開放しなければならない。
// -------------------------------------------------------------
LPVOID LoadPEImage(PPE_HEADERS pe, 
				   LPVOID lpFileImage, 
				   DWORD dwMemSize)
{
	// メモリを確保
	LPVOID lpMemImage = (PTCHAR)VirtualAlloc(
		NULL, dwMemSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(lpMemImage == NULL)
		return NULL;

	PTCHAR szInPtr  = (PTCHAR)lpFileImage;
	PTCHAR szOutPtr = (PTCHAR)lpMemImage;

	// ヘッダをメモリイメージとしてコピー
	DWORD dwHeaderSize = pe->poh->SizeOfHeaders;
	for(int i=0; i < pe->pfh->NumberOfSections; i++){
		if(pe->psh[i].PointerToRawData < dwHeaderSize)
			dwHeaderSize = pe->psh[i].PointerToRawData;
	}
	CopyMemory(szOutPtr, szInPtr, dwHeaderSize);

	// ポインタ移動
	szOutPtr += GetAlignedSize(
		pe->poh->SizeOfHeaders, pe->poh->SectionAlignment);
	
	// 全セクションをメモリイメージとしてコピー
	for(int i=0; i < pe->pfh->NumberOfSections; i++){
		
		// セクションサイズが0以下ならば無視
		if(pe->psh[i].SizeOfRawData <= 0){
			// もしVirtualSizeがあるなら更新
			if(pe->psh[i].Misc.VirtualSize){
				szInPtr += GetAlignedSize(
					pe->psh[i].Misc.VirtualSize, 
					pe->poh->SectionAlignment);
			}
			continue;
		}
		
		// セクションの読み込みサイズを取得
		DWORD dwToRead = pe->psh[i].SizeOfRawData;
		if(dwToRead > pe->psh[i].Misc.VirtualSize)
			dwToRead = pe->psh[i].Misc.VirtualSize;

		// 1セクションをメモリイメージとしてコピー
		szInPtr = (PTCHAR)lpFileImage + pe->psh[i].PointerToRawData;
		CopyMemory(szOutPtr, szInPtr, dwToRead);
		
		// ポインタ移動
		szOutPtr += GetAlignedSize(
			pe->psh[i].Misc.VirtualSize, pe->poh->SectionAlignment);
	}

	// メモリポインタを返す
	return lpMemImage;
}

// -------------------------------------------------------------
// LoadPEImage関数にて確保されたメモリ領域を開放する関数
//   引数：lpMem（メモリイメージのポインタ）
//         dwMemSize（メモリイメージのサイズ）
// 戻り値：なし
// -------------------------------------------------------------
VOID FreePEImage(LPVOID lpMem, DWORD dwMemSize)
{
	VirtualFree(lpMem, dwMemSize, MEM_DECOMMIT);
}

// ZwUnmapViewOfSection関数呼び出しのための定義
typedef DWORD (WINAPI *PTRZwUnmapViewOfSection)(IN HANDLE, IN PVOID);

// プロセス情報の構造体
typedef struct {
	DWORD    dwBaseAddr;
	DWORD    dwImageSize;
} PROCINFO, *PPROCINFO;

// -------------------------------------------------------------
// メモリイメージを実行する関数
//   引数：pe（PEファイルのヘッダポインタ構造体のポインタ）
//         pMemImage     （メモリイメージのポインタ）
//         dwMemImageSize（メモリイメージのサイズ）
//         szTargetProc  （ターゲットプロセスのパス）
// 戻り値：成功時0、失敗時0以外
// -------------------------------------------------------------
int ExecPEImage(PPE_HEADERS pe, 
				LPVOID pMemImage,
				DWORD dwMemImageSize,
				PTCHAR szTargetProc)
{
	// プロセス生成
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	BOOL bFlag = CreateProcess(NULL, szTargetProc,
		NULL, NULL, 0, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
	if( ! bFlag)
		return -1;

	// 生成したプロセス内のスレッドコンテキスト取得
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;
	GetThreadContext(pi.hThread, &ctx);

	PROCINFO piChildInfo;
	DWORD *pebInfo = (DWORD *)ctx.Ebx;

	// 生成したプロセスのベースアドレスを取得
	DWORD dwRead;
	ReadProcessMemory(pi.hProcess, &pebInfo[2], 
		(LPVOID)&(piChildInfo.dwBaseAddr), sizeof(DWORD), &dwRead);
	
	DWORD dwCurAddr = piChildInfo.dwBaseAddr;
	// フリーに扱えるメモリ領域を検索
	MEMORY_BASIC_INFORMATION mbiInfo;
	while(VirtualQueryEx(pi.hProcess, 
		(LPVOID)dwCurAddr, &mbiInfo, sizeof(mbiInfo)))
	{
		if(mbiInfo.State == MEM_FREE)
			break;
		dwCurAddr += mbiInfo.RegionSize;
	}
	
	// 生成したプロセスが割り当ているメモリサイズを取得
	piChildInfo.dwImageSize = dwCurAddr - (DWORD)piChildInfo.dwBaseAddr;

	LPVOID lpProcessMem;
	
	// 生成されたプロセスのメモリ空間に収まるならそのまま利用する
	if(pe->poh->ImageBase == piChildInfo.dwBaseAddr && 
		dwMemImageSize <= piChildInfo.dwImageSize)
	{
		lpProcessMem = (LPVOID)piChildInfo.dwBaseAddr;
		DWORD dwOldProtect;
		VirtualProtectEx(pi.hProcess, 
			(LPVOID)piChildInfo.dwBaseAddr, piChildInfo.dwImageSize, 
			PAGE_EXECUTE_READWRITE, &dwOldProtect);
	
	// 生成されたプロセスのメモリ空間に収まらないならば
	// 新しくメモリ空間を割り当てる
	}else{
		// ZwUnmapViewOfSection関数は
		// プロセスの仮想アドレス空間にマップされたビューを解放する
		PTRZwUnmapViewOfSection pZwUnmapViewOfSection = 
			(PTRZwUnmapViewOfSection)GetProcAddress(
			GetModuleHandle("ntdll.dll"), "ZwUnmapViewOfSection");
		// メモリを開放
		if(pZwUnmapViewOfSection(
			pi.hProcess, (LPVOID)piChildInfo.dwBaseAddr) == 0)
		{
			// 強引にメモリ空間を確保する
			lpProcessMem = VirtualAllocEx(pi.hProcess, 
				(LPVOID)pe->poh->ImageBase, dwMemImageSize, 
				MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			if( ! lpProcessMem)
				return -1;
		}
	}
	
	pebInfo = (DWORD *)ctx.Ebx;

	// プロセスのベースアドレスを書き換える
	DWORD dwWrote;
	WriteProcessMemory(pi.hProcess, &pebInfo[2], 
		&lpProcessMem, sizeof(DWORD), &dwWrote);

	// メモリイメージをプロセス空間へコピー
	if( ! WriteProcessMemory(pi.hProcess, 
		lpProcessMem, pMemImage, dwMemImageSize, NULL))
	{
		TerminateProcess(pi.hProcess, 0);
		return -1;
	}
	
	// プログラムのエントリポイントをEAXへ
	if((DWORD)lpProcessMem == piChildInfo.dwBaseAddr)
		ctx.Eax = pe->poh->ImageBase + pe->poh->AddressOfEntryPoint;
	else
		ctx.Eax = (DWORD)lpProcessMem + pe->poh->AddressOfEntryPoint;

	// コンテキストをセットしてプロセスを開始
	ctx.ContextFlags = CONTEXT_FULL;
	SetThreadContext(pi.hThread, &ctx);
	ResumeThread(pi.hThread);
	return 0;
}
