; -----------------------------------------------------------------------
; pm_utils.S — Protected mode utility library (fixed & optimized version)
; NASM syntax, 32-bit protected-mode helpers
; -----------------------------------------------------------------------

%define SCREEN_WIDTH     80
%define SCREEN_HEIGHT    25

%define ATA_PRIMARY_BASE     0x1F0
%define ATA_PRIMARY_CONTROL  0x3F6

%define WHITE_ON_BLACK 0x0F

section .data
cursor_row: dd 0
cursor_col: dd 0

section .text

; ======================================================================
; 光标管理函数 (cdecl)
; ======================================================================

set_cursor_pos:
    ; [esp+4] = row, [esp+8] = col
    mov eax, [esp + 4]
    mov [cursor_row], eax
    mov eax, [esp + 8]
    mov [cursor_col], eax
    ret

get_cursor_pos:
    mov eax, [cursor_row]
    mov edx, [cursor_col]
    ret

move_cursor_forward:
    ; [esp+4] = count
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]      ; count
    mov ecx, [cursor_row]
    mov edx, [cursor_col]
    add edx, eax

.fixup:
    cmp edx, SCREEN_WIDTH
    jl .store
    sub edx, SCREEN_WIDTH
    inc ecx
    jmp .fixup
.store:
    mov [cursor_row], ecx
    mov [cursor_col], edx
    pop ebp
    ret

cursor_newline:
    ; 行 +1，列 = 0，防止越界（不自动滚屏）
    mov eax, [cursor_row]
    inc eax
    cmp eax, SCREEN_HEIGHT
    jb .ok
    mov eax, SCREEN_HEIGHT - 1
.ok:
    mov [cursor_row], eax
    mov dword [cursor_col], 0
    ret

; ======================================================================
; 硬件光标
; ======================================================================

set_hardware_cursor:
    push ebp
    mov  ebp, esp
    push eax
    push ebx
    push ecx
    push edx

    mov eax, [ebp + 8]      ; row
    mov ecx, SCREEN_WIDTH
    xor edx, edx
    mul ecx
    add eax, [ebp + 12]     ; + col

    mov dx, 0x3D4
    mov bx, ax              ; bx = cell index (word)

    mov al, 0x0E
    out dx, al
    inc dx
    mov al, bh
    out dx, al

    dec dx
    mov al, 0x0F
    out dx, al
    inc dx
    mov al, bl
    out dx, al

    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret

sync_hardware_cursor:
    ; 使用当前软光标同步硬件光标
    mov eax, [cursor_row]
    mov edx, [cursor_col]
    push edx
    push eax
    call set_hardware_cursor
    add esp, 8
    ret

; ======================================================================
; 屏幕控制
; ======================================================================

clear_screen:
    push ebx
    push ecx
    push edi
    push esi

    mov ecx, SCREEN_WIDTH * SCREEN_HEIGHT
    mov bx, 0x0720          ; word: attr + ' '
    xor edi, edi
    mov esi, ecx

.clear_loop:
    mov [gs:edi], bx
    add edi, 2
    dec esi
    jnz .clear_loop

    pop esi
    pop edi
    pop ecx
    pop ebx

    mov dword [cursor_row], 0
    mov dword [cursor_col], 0

    push 0
    push 0
    call set_hardware_cursor
    add esp, 8
    ret

; ======================================================================
; 打印功能
; ======================================================================

; get_vram_addr - 返回字节偏移到 eax（字节偏移，用于 [gs:eax] 访问）
get_vram_addr:
    mov eax, [cursor_row]
    mov ecx, SCREEN_WIDTH
    xor edx, edx
    mul ecx
    add eax, [cursor_col]
    shl eax, 1              ; bytes
    ret

print_char:
    push ebp
    mov ebp, esp
    push edi
    push eax
    push ebx
    push ecx
    push edx

    ; 取参数（只用低字节）
    mov  al, byte [ebp + 8]    ; char
    mov  bl, byte [ebp + 12]   ; attr

    ; 把 char/attr 合成 word，保存到 BX，避免被后续计算覆盖
    mov  ah, bl                ; AH = attr, AL = char
    mov  bx, ax                ; BX = AX(=char+attr)

    ; 计算显存偏移 (用 EAX 重新计算，不影响 BX)
    mov eax, [cursor_row]
    mov ecx, SCREEN_WIDTH
    xor edx, edx
    mul ecx
    add eax, [cursor_col]
    shl eax, 1
    mov edi, eax

    ; 写显存
    mov ax, bx
    mov [gs:edi], ax

    ; 更新光标
    inc dword [cursor_col]
    cmp dword [cursor_col], SCREEN_WIDTH
    jl .done
    mov dword [cursor_col], 0
    inc dword [cursor_row]
    cmp dword [cursor_row], SCREEN_HEIGHT
    jl .done
    mov dword [cursor_row], SCREEN_HEIGHT - 1

.done:
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop edi
    pop ebp
    ret

print_string:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push ebx

    mov esi, [ebp + 8]        ; string
    mov bl, [ebp + 12]        ; attr

    mov eax, [cursor_row]
    mov ecx, SCREEN_WIDTH
    xor edx, edx
    mul ecx                   ; eax = row * SCREEN_WIDTH
    add eax, [cursor_col]     ; eax = 偏移（单位：字符）

    mov edi, eax
    shl edi, 1                ; 每个字符 2 字节
    mov ah, bl                ; attr 在 ah

.print_loop:
    lodsb
    test al, al
    jz .done
    cmp al, 10                ; '\n'
    je .newline

    mov [gs:edi], ax
    add edi, 2

    ; 更新 cursor_col
    inc dword [cursor_col]
    cmp dword [cursor_col], SCREEN_WIDTH
    jl .print_loop

.newline:
    mov dword [cursor_col], 0
    inc dword [cursor_row]
    mov eax, [cursor_row]
    mov ecx, SCREEN_WIDTH
    xor edx, edx
    mul ecx
    shl eax, 1
    mov edi, eax
    jmp .print_loop

.done:
    pop ebx
    pop edi
    pop esi
    pop ebp
    ret

println:
    push ebp
    mov ebp, esp

    ; 取出参数
    mov eax, [ebp + 8]      ; string
    mov edx, [ebp + 12]     ; color

    ; 正确传递给 print_string
    push edx
    push eax
    call print_string
    add esp, 8

    call cursor_newline
    call sync_hardware_cursor

    pop ebp
    ret

; ======================================================================
; 打印十六进制/十六进制32
; ======================================================================

; print_hex8: [esp+4]=value(byte), [esp+8]=attr(byte)
print_hex8:
    push ebp
    mov  ebp, esp
    push eax
    push ebx
    push ecx
    push edx

    mov  al, byte [ebp + 8]     ; value (low 8 bits)
    mov  bl, byte [ebp + 12]    ; attr
    mov  dl, al                 ; backup original byte into DL

    ; ---- high nibble ----
    mov  cl, dl
    shr  cl, 4
    and  cl, 0x0F
    cmp  cl, 9
    jbe  .h_digit
    add  cl, 'A' - 10
    jmp  .h_done
.h_digit:
    add  cl, '0'
.h_done:
    movzx eax, cl        ; zero-extend ASCII char into EAX
    movzx ecx, bl        ; zero-extend attr into ECX
    push ecx
    push eax
    call print_char
    add  esp, 8

    ; ---- low nibble ----
    mov  cl, dl
    and  cl, 0x0F
    cmp  cl, 9
    jbe  .l_digit
    add  cl, 'A' - 10
    jmp  .l_done
.l_digit:
    add  cl, '0'
.l_done:
    movzx eax, cl
    movzx ecx, bl
    push ecx
    push eax
    call print_char
    add  esp, 8

    pop  edx
    pop  ecx
    pop  ebx
    pop  eax
    pop  ebp
    ret

; print_hex32: [esp+4]=value(dword), [esp+8]=attr(byte)
print_hex32:
    push    ebp
    mov     ebp, esp
    push    eax
    push    ebx
    push    ecx
    push    edx
    push    esi
    push    edi

    mov     eax, [ebp + 8]     ; value
    mov     bl, byte [ebp + 12] ; attr
    mov     ecx, 8

.hloop:
    mov     edx, eax
    shr     edx, 28
    and     dl, 0x0F

    cmp     dl, 9
    jbe     .digit
    add     dl, 'A' - 10
    jmp     .out
.digit:
    add     dl, '0'
.out:
    movzx   esi, dl
    movzx   edi, bl
    push    edi
    push    esi
    call    print_char
    add     esp, 8

    shl     eax, 4
    loop    .hloop

    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax
    pop     ebp
    ret

; ======================================================================
; 内存操作（标准 prologue/epilogue）
; ======================================================================

memcpy:
    ; [esp+4] = dest, [esp+8] = src, [esp+12] = n
    push ebp
    mov  ebp, esp
    push esi
    push edi
    push ecx

    mov edi, [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]
    cld
    rep movsb

    pop ecx
    pop edi
    pop esi
    pop ebp
    ret

memset:
    ; [esp+4]=dest, [esp+8]=val (byte), [esp+12]=n
    push ebp
    mov  ebp, esp
    push edi
    push ecx

    mov edi, [ebp + 8]      ; dest
    mov al, [ebp + 12]      ; val (byte)
    mov ecx, [ebp + 16]     ; count
    cld
    rep stosb

    pop ecx
    pop edi
    pop ebp
    ret

; ======================================================================
; ATA 磁盘读取 (PIO 模式)
; 约定: eax = start LBA (28-bit), ebx = buf, ecx = count
; 返回: eax = 实际读取扇区数
; ======================================================================
read_disk:
    push ebp
    mov  ebp, esp
    pushad

    mov esi, [ebp + 8]     ; LBA 起始扇区号
    mov edi, [ebp + 12]    ; 扇区数量
    mov ebx, [ebp + 16]    ; 目标缓冲区地址

.next_sector:
    ; --------------------------------------
    ; 写入扇区数量（每次读1个）
    ; --------------------------------------
    mov dx, ATA_PRIMARY_BASE + 2
    mov al, 1
    out dx, al

    ; --------------------------------------
    ; 写入 LBA 地址（低 24 位）
    ; --------------------------------------
    mov eax, esi
    mov dx, ATA_PRIMARY_BASE + 3
    out dx, al
    shr eax, 8
    mov dx, ATA_PRIMARY_BASE + 4
    out dx, al
    shr eax, 8
    mov dx, ATA_PRIMARY_BASE + 5
    out dx, al
    shr eax, 8
    and al, 0x0F
    or  al, 0xE0             ; 主盘 + LBA 模式
    mov dx, ATA_PRIMARY_BASE + 6
    out dx, al

    ; --------------------------------------
    ; 发送读命令 READ SECTOR (0x20)
    ; --------------------------------------
    mov dx, ATA_PRIMARY_BASE + 7
    mov al, 0x20
    out dx, al

.wait_ready:
    in  al, dx                       ; 状态寄存器
    test al, 0x80                    ; BSY=1 ? 忙碌中？
    jnz  .wait_ready
    test al, 0x08                    ; DRQ=1 ? 数据准备好？
    jz   .wait_ready

    ; ----------------------- 读一个扇区 -----------------------
    mov dx, ATA_PRIMARY_BASE + 0     ; 数据端口
    mov ecx, 256                     ; 每扇区 256 words = 512B
.read_word:
    in  ax, dx
    mov [ebx], ax
    add ebx, 2
    loop .read_word

    ; ----------------------- 准备下一个扇区 -----------------------
    inc esi       ; LBA++
    dec edi       ; 扇区数--
    jnz .next_sector

.done:
    popad
    pop ebp
    ret