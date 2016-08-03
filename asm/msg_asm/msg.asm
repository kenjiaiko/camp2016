; -------------------------------------------------------------------
; msg.asm
; 2005/07/24 written by Kenji Aiko
; 
; compile:
; > nasmw -fwin32 msg.asm
; > golink -entry main msg.obj
; 
; NASM (http://nasm.sourceforge.net/)
; GOLINK (http://www.jorgon.freeserve.co.uk/)
; 
; -------------------------------------------------------------------

section .text
global main

main:

    push ebp
    mov ebp, esp
    sub esp, 100h

    ; [ebp] stack layout
    ; .bss
    ;   -00h ebpレジスタ値
    ;   -04h kernel32.dllのベースアドレス
    ;   -08h user32.dllのベースアドレス
    ;   -0Ch MessageBox関数のアドレス
    ;   -10h LoadLibrary関数のアドレス
    ;   -14h GetProcAddress関数のアドレス
    ;   -18h NumberOfNames
    ;   -1Ch AddressOfFunctions
    ;   -20h AddressOfNames
    ;   -24h AddressOfNameOrdinals
    ; .data
    ;   -34h 'GetProcAddressA', 0
    ;   -44h 'LoadLibraryA', 0
    ;   -54h 
    ;   -64h 'user32.dll', 0
    ;   -74h 'MessageBoxA', 0
    ;   -84h 'GetActiveWindow', 0
    ;   -A4h 'Wizard Bible vol.19', 0
    ;   -C4h 'Hello! This is Inject Code.', 0

    ; 'GetProcAddressA', 0
    mov eax, 050746547h
    mov dword [ebp - 34h], eax
    mov eax, 041636F72h
    mov dword [ebp - 30h], eax
    mov eax, 065726464h
    mov dword [ebp - 2Ch], eax
    mov eax, 000417373h
    mov dword [ebp - 28h], eax

    ; 'LoadLibraryA', 0
    mov eax, 064616F4Ch
    mov dword [ebp - 44h], eax
    mov eax, 07262694Ch
    mov dword [ebp - 40h], eax
    mov eax, 041797261h
    mov dword [ebp - 3Ch], eax
    mov eax, 000000000h
    mov dword [ebp - 38h], eax

    ; 'ExitProcess', 0
    mov eax, 074697845h
    mov dword [ebp - 54h], eax
    mov eax, 0636F7250h
    mov dword [ebp - 50h], eax
    mov eax, 000737365h
    mov dword [ebp - 4Ch], eax
    mov eax, 000000000h
    mov dword [ebp - 48h], eax

    ; 'user32.dll', 0
    mov eax, 072657375h
    mov dword [ebp - 64h], eax
    mov eax, 0642E3233h
    mov dword [ebp - 60h], eax
    mov eax, 000006C6Ch
    mov dword [ebp - 5Ch], eax
    mov eax, 000000000h
    mov dword [ebp - 58h], eax
    
    ; 'MessageBoxA', 0
    mov eax, 07373654Dh
    mov dword [ebp - 74h], eax
    mov eax, 042656761h
    mov dword [ebp - 70h], eax
    mov eax, 00041786Fh
    mov dword [ebp - 6Ch], eax
    mov eax, 000000000h
    mov dword [ebp - 68h], eax

    ; 'GetActiveWindow', 0
    mov eax, 041746547h
    mov dword [ebp - 84h], eax
    mov eax, 076697463h
    mov dword [ebp - 80h], eax
    mov eax, 06E695765h
    mov dword [ebp - 7Ch], eax
    mov eax, 000776F64h
    mov dword [ebp - 78h], eax

    ; 'Wizard Bible vol.19', 0
    mov eax, 0617A6957h
    mov dword [ebp - 0A4h], eax
    mov eax, 042206472h
    mov dword [ebp - 0A0h], eax
    mov eax, 0656C6269h
    mov dword [ebp - 9Ch], eax
    mov eax, 06C6F7620h
    mov dword [ebp - 98h], eax
    mov eax, 00039312Eh
    mov dword [ebp - 94h], eax

    ; 'Hello! This is Inject Code.', 0
    mov eax, 06C6C6548h
    mov dword [ebp - 0C4h], eax
    mov eax, 05420216Fh
    mov dword [ebp - 0C0h], eax
    mov eax, 020736968h
    mov dword [ebp - 0BCh], eax
    mov eax, 049207369h
    mov dword [ebp - 0B8h], eax
    mov eax, 063656A6Eh
    mov dword [ebp - 0B4h], eax
    mov eax, 06F432074h
    mov dword [ebp - 0B0h], eax
    mov eax, 0002E6564h
    mov dword [ebp - 0ACh], eax

    ; -------------------------------------------
    ; KERNEL32.DLLのベースアドレスを求める
    ; -------------------------------------------

    ; KERNEL32.DLLへのリターンアドレス（ebp + 4h）取得
    mov ebx, dword [ebp + 4]
    and ebx, 0FFFF0000h

    mov ecx, 5

.check_PE:

    ; 最初の2バイトが「MZ」であるならPEファイル判定処理へ
    cmp word [ebx], 5A4Dh
    je .check2

.check1:

   ; もしecx=0ならKERNEL32.DLLは発見できなかった
    sub ebx, 10000h
    dec ecx
    xor eax, eax
    cmp ecx, eax
    jne .check_PE
    jmp ExitProgram

.check2:

    ; アドレス3Chからの4バイトが50450000h（PE\0\0）
    ; となっているかどうかを調べる
    mov eax, dword [ebx + 3Ch]
    add eax, ebx
    cmp dword [eax], 00004550h
    jne .check1

got_kernel:

    ; KERNEL32.DLLのベースアドレスを取得
    mov dword [ebp - 4h], ebx

    ; -------------------------------------------
    ; エクスポートディレクトリテーブルを探索
    ; -------------------------------------------

    cld  ; DF = 0

    ;   -18h NumberOfNames
    ;   -1Ch AddressOfFunctions
    ;   -20h AddressOfNames
    ;   -24h AddressOfNameOrdinals

    mov esi, [eax + 78h]  ; エクスポートディレクトリテーブル
    add esi, [ebp - 4h]
    add esi, 18h  ; esi = NumberOfNamesのアドレス
    lodsd  ; [esi]の4バイトをeaxへ（ if DF=0 then esi+=4 ）
    mov dword [ebp - 18h], eax
    lodsd  ; [esi]の4バイトをeaxへ（ if DF=0 then esi+=4 ）
    add eax, [ebp - 4h]
    mov dword [ebp - 1Ch], eax
    lodsd  ; [esi]の4バイトをeaxへ（ if DF=0 then esi+=4 ）
    add eax, [ebp - 4h]
    mov dword [ebp - 20h], eax
    lodsd  ; [esi]の4バイトをeaxへ（ if DF=0 then esi+=4 ）
    add eax, [ebp - 4h]
    mov dword [ebp - 24h], eax

    ; -------------------------------------------
    ; GetProcAddressA関数とLoadLibraryA関数の検索
    ; -------------------------------------------

    ; フラグ
    xor edx, edx

search_Func:

    ; ediにAPI名の羅列の先頭アドレスを入れる
    mov eax, dword [ebp - 20h]
    mov edi, dword [eax]
    add edi, [ebp - 4h]

    cld  ; DF = 0
    xor ecx, ecx

.next:

    cmp dl, dh
    jnz .flag
    ; esiに"GetProcAddressA"文字列のアドレスを入れる
    lea esi, [ebp - 34h]
    jmp .check
.flag:
    ; esiに"LoadLibraryA"文字列のアドレスを入れる
    lea esi, [ebp - 44h]

.check:

    ; API名が一致しているかをチェックする
    ; 1文字比較
    ; cmp byte [esi], [edi]（ if DF=0 then esi++, edi++ ）
    cmpsb
    jne .nextAPI

    ; 文字列の終端ならば一致したAPIが見つかったのでgotへ
    ; 終端じゃなければcheckへ
    cmp byte [edi], 0
    je .got
    jmp .check

.nextAPI:

    ; 次のAPIへポインタを動かす
    inc ecx
    cmp ecx, dword [ebp - 18h]
    jnge .nextAPI_2
    jmp ExitProgram
.nextAPI_2:
    add eax, 4
    mov edi, dword [eax]
    add edi, [ebp - 4h]
    jmp .next

.got:

    ; GetProcAddress関数アドレス取得
    shl ecx, 1  ; 左シフト（ecx = ecx * 2）
    mov esi, dword [ebp - 24h]
    add esi, ecx
    xor eax, eax
    mov ax, word [esi]
    shl eax, 2  ; 左シフト（eax = eax * 4）
    mov esi, [ebp - 1Ch]
    add esi, eax
    mov edi, dword [esi]
    add edi, [ebp - 4h]

    cmp dl, dh
    jnz .end
    mov dword [ebp -14h], edi
    inc dl
    jmp search_Func
.end:
    mov dword [ebp -10h], edi

    ; -------------------------------------------
    ; 使用する関数のアドレス取得
    ; -------------------------------------------

get_Func:

    ; LoadLibraryを使ってuser32.dllのベースアドレス取得
    mov eax, ebp
    sub eax, 64h
    push eax
    call dword [ebp - 10h]
    mov dword [ebp - 08h], eax

    ; MessageBoxの関数のアドレス取得
    mov eax, ebp
    sub eax, 74h
    push eax
    push dword [ebp - 08h]
    call dword [ebp - 14h]
    mov dword [ebp - 0Ch], eax

    ; GetActiveWindowの関数のアドレス取得
    mov eax, ebp
    sub eax, 84h
    push eax
    push dword [ebp - 08h]
    call dword [ebp - 14h]

    ;   -04h kernel32.dllのベースアドレス
    ;   -08h user32.dllのベースアドレス
    ;   -0Ch MessageBox関数のアドレス
    ;   -10h LoadLibrary関数のアドレス
    ;   -14h GetProcAddress関数のアドレス

    ; -------------------------------------------
    ; プログラム終了
    ; -------------------------------------------

ExitProgram:
    ; ExitProcessの関数のアドレス取得
    mov eax, ebp
    sub eax, 54h
    push eax
    push dword [ebp - 08h]
    call dword [ebp - 14h]
    ; call ExitProcess
    push 1
    call eax
    ;
    mov esp, ebp
    pop ebp
    ret