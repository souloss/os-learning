; -----------------------------------------------------------------------
; pm_utils.S — Protected mode utility library (optimized version)
; 功能: 提供保护模式下的打印、内存操作和磁盘操作功能
; 调用约定: cdecl (参数通过堆栈传递，调用者清理堆栈)
; 寄存器使用: 不破坏ebx, esi, edi, ebp; 可修改eax, ecx, edx
; 前提: gs寄存器已设置为SELECTOR_VIDEO
; NASM syntax
; -----------------------------------------------------------------------

; =================================================================================
; 常量定义
; =================================================================================
SCREEN_WIDTH         equ 80          ; 屏幕宽度（字符）
SCREEN_HEIGHT        equ 25          ; 屏幕高度（字符）

; 磁盘操作常量
ATA_PRIMARY_BASE     equ 0x1F0      ; 主ATA控制器基地址
ATA_PRIMARY_CONTROL  equ 0x3F6      ; 主ATA控制器控制端口

; =================================================================================
; 数据段定义
; =================================================================================
section .data
cursor_row: dd 0                  ; 当前行 (0-24)
cursor_col: dd 0                  ; 当前列 (0-79)

; =================================================================================
; 代码段定义
; =================================================================================
section .text

; =================================================================================
; 软件光标操作函数
; =================================================================================

; -----------------------------------------------------------------------
; set_cursor_pos - 设置软件光标位置
; 输入: [esp+4] = 行, [esp+8] = 列
; 输出: 无
; 破坏: eax
; 副作用: 修改cursor_row和cursor_col
; -----------------------------------------------------------------------
set_cursor_pos:
    mov eax, [esp + 4]    ; 行
    mov [cursor_row], eax
    mov eax, [esp + 8]    ; 列
    mov [cursor_col], eax
    ret

; -----------------------------------------------------------------------
; get_cursor_pos - 获取软件光标位置
; 输入: 无
; 输出: eax = 行, edx = 列
; 破坏: eax, edx
; 副作用: 无
; -----------------------------------------------------------------------
get_cursor_pos:
    mov eax, [cursor_row]
    mov edx, [cursor_col]
    ret

; -----------------------------------------------------------------------
; move_cursor_forward - 向前移动光标指定字符数
; 输入: [esp+4] = 字符数
; 输出: 无
; 破坏: eax, ecx, edx
; 副作用: 修改cursor_row和cursor_col
; -----------------------------------------------------------------------
move_cursor_forward:
    mov eax, [esp + 4]    ; 字符数
    mov ecx, [cursor_row]
    mov edx, [cursor_col]
    add edx, eax
    
.start_new_row:
    cmp edx, SCREEN_WIDTH
    jle .do_move
    inc ecx
    sub edx, SCREEN_WIDTH
    jmp .start_new_row
    
.do_move:
    mov [cursor_row], ecx
    mov [cursor_col], edx
    ret

; -----------------------------------------------------------------------
; cursor_newline - 光标换行
; 输入: 无
; 输出: 无
; 破坏: eax
; 副作用: 修改cursor_row和cursor_col
; -----------------------------------------------------------------------
cursor_newline:
    mov eax, [cursor_row]
    inc eax
    mov [cursor_row], eax
    mov dword [cursor_col], 0
    ret

; =================================================================================
; 硬件光标操作函数
; =================================================================================

; -----------------------------------------------------------------------
; set_hardware_cursor - 设置硬件光标位置
; 输入: [esp+4] = 行, [esp+8] = 列
; 输出: 无
; 破坏: eax, ecx, edx
; 副作用: 无
; -----------------------------------------------------------------------
set_hardware_cursor:
    ; 计算光标位置
    mov eax, [esp + 4]    ; 行
    mov ecx, SCREEN_WIDTH
    mul ecx
    add eax, [esp + 8]    ; 加上列
    shl eax, 1           ; 转换为字节偏移
    
    ; 设置高字节
    mov ecx, eax
    shr ecx, 8
    mov al, 14
    mov dx, 0x3D4
    out dx, al
    mov al, cl
    mov dx, 0x3D5
    out dx, al
    
    ; 设置低字节
    mov al, 15
    mov dx, 0x3D4
    out dx, al
    mov al, ch
    mov dx, 0x3D5
    out dx, al
    
    ret

; -----------------------------------------------------------------------
; sync_hardware_cursor - 同步硬件光标到软件光标位置
; 输入: 无
; 输出: 无
; 破坏: eax, ecx, edx
; 副作用: 无
; -----------------------------------------------------------------------
sync_hardware_cursor:
    mov eax, [cursor_row]
    mov edx, [cursor_col]
    push edx
    push eax
    call set_hardware_cursor
    add esp, 8
    ret

; =================================================================================
; 屏幕控制函数
; =================================================================================

; -----------------------------------------------------------------------
; clear_screen - 清除屏幕并重置光标位置
; 输入: 无
; 输出: 无
; 破坏: eax, ecx, edi
; 副作用: 重置软件和硬件光标位置
; -----------------------------------------------------------------------
clear_screen:
    ; 填充整个屏幕为空格
    xor edi, edi              ; gs:edi = 视频内存起始地址
    mov ecx, SCREEN_WIDTH * SCREEN_HEIGHT
    mov eax, 0x0720           ; 空格字符，黑底白字
    cld
    rep stosw                 ; 填充整个屏幕
    
    ; 重置软件光标位置
    mov dword [cursor_row], 0
    mov dword [cursor_col], 0
    
    ; 重置硬件光标位置
    push 0
    push 0
    call set_hardware_cursor
    add esp, 8
    
    ret

; =================================================================================
; 打印功能函数
; =================================================================================

; -----------------------------------------------------------------------
; print_char - 打印单个字符
; 输入: [esp+4] = 字符, [esp+8] = 属性
; 输出: 无
; 破坏: eax, ecx
; 副作用: 更新光标位置
; -----------------------------------------------------------------------
print_char:
    mov eax, [esp + 4]    ; 字符
    mov ah, [esp + 8]     ; 属性
    
    ; 计算视频内存地址
    call get_vram_addr
    mov [gs:eax], ax      ; 写入字符和属性
    
    ; 移动光标
    push 1
    call move_cursor_forward
    add esp, 4
    
    ret

; -----------------------------------------------------------------------
; print_string - 打印字符串
; 输入: [esp+4] = 字符串地址, [esp+8] = 属性
; 输出: 无
; 破坏: eax, ecx, esi
; 副作用: 更新光标位置
; -----------------------------------------------------------------------
print_string:
    mov esi, [esp + 4]    ; 字符串地址
    mov ah, [esp + 8]     ; 属性
    
.print_loop:
    mov al, [esi]
    test al, al
    jz .done
    
    push eax
    push eax
    call print_char
    add esp, 8
    
    inc esi
    jmp .print_loop
    
.done:
    ret

; -----------------------------------------------------------------------
; print_hex8 - 打印8位十六进制数字
; 输入: [esp+4] = 数字, [esp+8] = 属性
; 输出: 无
; 破坏: eax, ecx
; 副作用: 更新光标位置
; -----------------------------------------------------------------------
print_hex8:
    mov al, [esp + 4]    ; 数字
    mov ah, [esp + 8]     ; 属性
    
    ; 打印高4位
    mov ecx, eax
    shr cl, 4
    and cl, 0x0F
    add cl, '0'
    cmp cl, '9'
    jbe .1
    add cl, 7            ; 'A' - '9' - 1
.1:
    push ecx
    push ecx
    call print_char
    add esp, 8
    
    ; 打印低4位
    mov cl, [esp + 4]    ; 原始数字
    and cl, 0x0F
    add cl, '0'
    cmp cl, '9'
    jbe .2
    add cl, 7            ; 'A' - '9' - 1
.2:
    push ecx
    push ecx
    call print_char
    add esp, 8
    
    ret

; -----------------------------------------------------------------------
; print_hex32 - 打印32位十六进制数字
; 输入: [esp+4] = 数字, [esp+8] = 属性
; 输出: 无
; 破坏: eax, ecx
; 副作用: 更新光标位置
; -----------------------------------------------------------------------
print_hex32:
    mov eax, [esp + 4]   ; 数字
    mov ah, [esp + 8]    ; 属性
    
    ; 打印最高字节
    push eax
    push eax
    call print_hex8
    add esp, 8
    
    ; 打印次高字节
    pop eax
    mov al, al
    push eax
    push eax
    call print_hex8
    add esp, 8
    
    ; 打印次低字节
    pop eax
    mov al, ah
    push eax
    push eax
    call print_hex8
    add esp, 8
    
    ; 打印最低字节
    pop eax
    mov al, al
    push eax
    push eax
    call print_hex8
    add esp, 8
    
    ret

; -----------------------------------------------------------------------
; print_dec32 - 打印32位十进制数字
; 输入: [esp+4] = 数字, [esp+8] = 属性
; 输出: 无
; 破坏: eax, ecx, edx
; 副作用: 更新光标位置
; -----------------------------------------------------------------------
print_dec32:
    mov eax, [esp + 4]   ; 数字
    mov ah, [esp + 8]    ; 属性
    
    ; 处理0的特殊情况
    test eax, eax
    jnz .not_zero
    push '0'
    push eax
    call print_char
    add esp, 8
    ret
    
.not_zero:
    push ebx              ; 保存ebx
    mov ebx, esp
    sub esp, 32          ; 分配缓冲区
    
    mov ecx, 10
    xor edx, edx
    
.div_loop:
    div ecx              ; eax = eax / 10, edx = eax % 10
    add dl, '0'
    mov [ebx + edx], dl
    test eax, eax
    jnz .div_loop
    
    ; 打印数字
    mov esi, ebx
.print_loop:
    lodsb
    test al, al
    jz .done
    push eax
    push eax
    call print_char
    add esp, 8
    jmp .print_loop
    
.done:
    mov esp, ebx
    pop ebx               ; 恢复ebx
    ret

; -----------------------------------------------------------------------
; println - 打印字符串并换行
; 输入: [esp+4] = 字符串地址, [esp+8] = 属性
; 输出: 无
; 破坏: eax, ecx, esi
; 副作用: 更新光标位置
; -----------------------------------------------------------------------
println:
    call print_string
    call cursor_newline
    call sync_hardware_cursor
    ret

; =================================================================================
; 辅助函数
; =================================================================================

; -----------------------------------------------------------------------
; get_vram_addr - 获取当前光标对应的视频内存地址
; 输入: 无
; 输出: eax = 视频内存地址
; 破坏: eax, ecx
; 副作用: 无
; -----------------------------------------------------------------------
get_vram_addr:
    mov eax, [cursor_row]
    mov ecx, SCREEN_WIDTH
    mul ecx
    add eax, [cursor_col]
    shl eax, 1              ; 转换为字节偏移
    ret

; =================================================================================
; 内存操作函数
; =================================================================================

; -----------------------------------------------------------------------
; memcpy - 内存拷贝
; 输入: [esp+4] = 目标地址, [esp+8] = 源地址, [esp+12] = 大小
; 输出: 无
; 破坏: eax, ecx, esi, edi
; 副作用: 无
; -----------------------------------------------------------------------
memcpy:
    push esi
    push edi
    
    mov edi, [esp + 12]   ; 目标地址
    mov esi, [esp + 16]   ; 源地址
    mov ecx, [esp + 20]   ; 大小
    cld
    rep movsb
    
    pop edi
    pop esi
    ret

; -----------------------------------------------------------------------
; memset - 设置内存区域
; 输入: [esp+4] = 目标地址, [esp+8] = 值, [esp+12] = 大小
; 输出: 无
; 破坏: eax, ecx, edi
; 副作用: 无
; -----------------------------------------------------------------------
memset:
    push edi
    
    mov edi, [esp + 8]    ; 目标地址
    mov al, [esp + 12]    ; 值
    mov ecx, [esp + 16]   ; 大小
    rep stosb
    
    pop edi
    ret

; =================================================================================
; 磁盘操作函数
; =================================================================================

; -----------------------------------------------------------------------
; read_disk - 从磁盘读取扇区
; 输入: eax = 起始LBA, ebx = 缓冲区地址, ecx = 扇区数
; 输出: eax = 实际读取的扇区数
; 破坏: eax, ecx, edx
; 副作用: 无
; -----------------------------------------------------------------------
read_disk:
    push esi
    push edi
    
    mov esi, eax          ; 保存LBA
    mov edi, ecx          ; 保存扇区数
    
    ; 设置扇区数
    mov al, cl
    mov dx, ATA_PRIMARY_BASE + 2
    out dx, al
    
    ; 设置LBA地址
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
    or al, 0xE0
    mov dx, ATA_PRIMARY_BASE + 6
    out dx, al
    
    ; 发送读取命令
    mov dx, ATA_PRIMARY_BASE + 7
    mov al, 0x20
    out dx, al
    
    ; 等待磁盘就绪
.wait_ready:
    in al, dx
    and al, 0x88
    cmp al, 0x08
    jnz .wait_ready
    
    ; 读取数据
    mov ecx, edi
    mov dx, ATA_PRIMARY_BASE
    
.read_loop:
    push ecx
    mov ecx, 256
.read_sector:
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop .read_sector
    pop ecx
    loop .read_loop
    
    mov eax, edi          ; 返回读取的扇区数
    
    pop edi
    pop esi
    ret