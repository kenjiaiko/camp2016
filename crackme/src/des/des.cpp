#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "des.h"
#include "crypt.h"

// --- DES暗号化処理を施す関数 ---
// szStrKeyに8バイト（64ビット）のキーのポインタ
// bDataに8バイトの倍数個の暗号文、または平文
// dwSizeにbDataのバイト単位のサイズ
// fFlagがtrueなら暗号化、falseなら復号化
VOID ReverseData(TCHAR *szStrKey, 
				 BYTE *bData, 
				 DWORD dwSize, 
				 BOOL bFlag)
{
	int i, j, k, t, u;

	// バイトデータからビットデータへ変換
	// （key配列にビットデータが格納される）
	BYTE key[64];
	for(i = 0; i < 8; i++)
		for(j = (i * 8), k = 7; j < (i * 8 + 8); j++, k--)
			key[j] = (szStrKey[i] >> k) & 01;

	BYTE block[64], dmy_block[64], KS[16][48], preS[48];
	BYTE C[28], D[28], L[32], R[32], DMY[32], f[32];

	// --- 内部鍵生成処理 ---

	// キーをblockにコピー
	for(i=0; i < 64; i++)
		block[i] = key[i];

	// 縮約型転置 PC1
	for(i=0; i < 28; i++){
		C[i] = block[PC1_C[i]-1];
		D[i] = block[PC1_D[i]-1];
	}

	for(i=0; i < 16; i++){
		// 循環シフト
		for(k=0; k < shift[i]; k++){
			t = C[0];
			for(j=0; j < 28-1; j++)
				C[j] = C[j+1];
			C[27] = t;

			t = D[0];
			for(j=0; j < 28-1; j++)
				D[j] = D[j+1];
			D[27] = t;
		}
		// 縮約型転置 PC2
		for(j=0; j < 24; j++){
			KS[i][j] = C[PC2_C[j]-1];
			KS[i][j+24] = D[PC2_D[j]-28-1];
		}
	}
	// 内部鍵生成完了！

	// --- 暗号化or復号化開始 ---

	for(u=0; u < (int)(dwSize / 8); u++){

		// bDataの8バイト（64ビット）をblockへ格納
		for(i = 0; i < 8; i++)
			for(j = (i * 8), k = 7; j < (i * 8 + 8); j++, k--)
				block[j] = (bData[u * 8 + i] >> k) & 01;

		// 初期転置
		for(i=0; i < 32; i++)
			L[i] = block[IP[i]-1];

		for(i=32; i < 64; i++)
			R[i-32] = block[IP[i]-1];

		// 16段の暗号化オペレーション
		for(i=0; i < 16; i++){

			for(j=0; j < 32; j++)
				DMY[j] = R[j];

			// 内部鍵との排他的論理和
			for(j=0; j < 48; j++){
				if(bFlag)
					preS[j] = R[e2[j]-1] ^ KS[i][j];
				else
					preS[j] = R[e2[j]-1] ^ KS[15-i][j];
			}

			// S BOX
			for(j=0; j < 8; j++){
				t = 6 * j;
				k = S[j][((
					(preS[t+0]*2) + (preS[t+5]*1))*16) + 
					((preS[t+1]*8) + (preS[t+2]*4) + 
					(preS[t+3]*2) + (preS[t+4]*1))];
                t = 4*j;
				f[t+0] = (k>>3)&01;
				f[t+1] = (k>>2)&01;
				f[t+2] = (k>>1)&01;
				f[t+3] = (k>>0)&01;
			}

			// 転置 P を行ったあとの 排他的論理和
			for(j=0; j < 32; j++)
				R[j] = f[P[j]-1] ^ L[j];

			for(j=0; j < 32; j++)
				L[j] = DMY[j];
		}

		// LとRを交換
		for(i=0; i < 32; i++){
			t = L[i];
			L[i] = R[i];
			R[i] = t;
		}

		for(i=0; i < 32; i++)
			dmy_block[i] = L[i];

		for(i=32; i < 64; i++)
			dmy_block[i] = R[i-32];

		// 最終転置
		for(i=0; i < 64; i++)
			block[i] = dmy_block[FP[i]-1];

		// blockのデータをbDataの8バイト（64ビット）へ格納
		for(i = 0; i < 8; i++){
			BYTE c = 0, e = 128;
			for(j = (i * 8); j < (i * 8 + 8); j++){
				c += (block[j] * e);
				e = e / 2;
			}
			bData[u * 8 + i] = c;
		}
	}
}

