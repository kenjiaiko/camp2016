#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "des.h"
#include "crypt.h"

// --- DES�Í����������{���֐� ---
// szStrKey��8�o�C�g�i64�r�b�g�j�̃L�[�̃|�C���^
// bData��8�o�C�g�̔{���̈Í����A�܂��͕���
// dwSize��bData�̃o�C�g�P�ʂ̃T�C�Y
// fFlag��true�Ȃ�Í����Afalse�Ȃ畜����
VOID ReverseData(TCHAR *szStrKey, 
				 BYTE *bData, 
				 DWORD dwSize, 
				 BOOL bFlag)
{
	int i, j, k, t, u;

	// �o�C�g�f�[�^����r�b�g�f�[�^�֕ϊ�
	// �ikey�z��Ƀr�b�g�f�[�^���i�[�����j
	BYTE key[64];
	for(i = 0; i < 8; i++)
		for(j = (i * 8), k = 7; j < (i * 8 + 8); j++, k--)
			key[j] = (szStrKey[i] >> k) & 01;

	BYTE block[64], dmy_block[64], KS[16][48], preS[48];
	BYTE C[28], D[28], L[32], R[32], DMY[32], f[32];

	// --- �������������� ---

	// �L�[��block�ɃR�s�[
	for(i=0; i < 64; i++)
		block[i] = key[i];

	// �k��^�]�u PC1
	for(i=0; i < 28; i++){
		C[i] = block[PC1_C[i]-1];
		D[i] = block[PC1_D[i]-1];
	}

	for(i=0; i < 16; i++){
		// �z�V�t�g
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
		// �k��^�]�u PC2
		for(j=0; j < 24; j++){
			KS[i][j] = C[PC2_C[j]-1];
			KS[i][j+24] = D[PC2_D[j]-28-1];
		}
	}
	// ���������������I

	// --- �Í���or�������J�n ---

	for(u=0; u < (int)(dwSize / 8); u++){

		// bData��8�o�C�g�i64�r�b�g�j��block�֊i�[
		for(i = 0; i < 8; i++)
			for(j = (i * 8), k = 7; j < (i * 8 + 8); j++, k--)
				block[j] = (bData[u * 8 + i] >> k) & 01;

		// �����]�u
		for(i=0; i < 32; i++)
			L[i] = block[IP[i]-1];

		for(i=32; i < 64; i++)
			R[i-32] = block[IP[i]-1];

		// 16�i�̈Í����I�y���[�V����
		for(i=0; i < 16; i++){

			for(j=0; j < 32; j++)
				DMY[j] = R[j];

			// �������Ƃ̔r���I�_���a
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

			// �]�u P ���s�������Ƃ� �r���I�_���a
			for(j=0; j < 32; j++)
				R[j] = f[P[j]-1] ^ L[j];

			for(j=0; j < 32; j++)
				L[j] = DMY[j];
		}

		// L��R������
		for(i=0; i < 32; i++){
			t = L[i];
			L[i] = R[i];
			R[i] = t;
		}

		for(i=0; i < 32; i++)
			dmy_block[i] = L[i];

		for(i=32; i < 64; i++)
			dmy_block[i] = R[i-32];

		// �ŏI�]�u
		for(i=0; i < 64; i++)
			block[i] = dmy_block[FP[i]-1];

		// block�̃f�[�^��bData��8�o�C�g�i64�r�b�g�j�֊i�[
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

