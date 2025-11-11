; -----------------------------------------------------------------------
; realmode_debug.asm — Real mode debug and development helper library
; NASM syntax
; -----------------------------------------------------------------------

; =================================================================================
; 数据段定义
; =================================================================================
section .data
cursor_x: db 0  ; 当前列 (0-79)
cursor_y: db 0  ; 当前行 (0-24)

; =================================================================================
; 清屏函数
; 输入: 无
; 输出: 无
; 破坏: AX, BX, CX, DI
; -----------------------------------------------------------------------
clear_screen:
    pusha
    mov ax, 0xB800      ; 视频内存段
    mov es, ax
    xor di, di          ; ES:DI = 视频内存起始地址
    mov cx, 80 * 25     ; 屏幕字符数
    mov ax, 0x0720      ; 空格字符，黑底白字
    cld
    rep stosw           ; 填充整个屏幕
    
    ; 重置光标位置
    mov byte [cs:cursor_x], 0
    mov byte [cs:cursor_y], 0
    
    ; 设置硬件光标到左上角
    mov dx, 0x03D4      ; CRTC地址端口
    mov al, 0x0E        ; 光标位置高字节寄存器
    out dx, al
    inc dx              ; 0x03D5 - CRTC数据端口
    mov al, 0
    out dx, al
    
    dec dx              ; 0x03D4 - CRTC地址端口
    mov al, 0x0F        ; 光标位置低字节寄存器
    out dx, al
    inc dx              ; 0x03D5 - CRTC数据端口
    mov al, 0
    out dx, al
    
    popa
    ret

; =================================================================================
; 设置光标位置函数
; 输入: DH = 行 (0-24), DL = 列 (0-79)
; 输出: 无
; 破坏: AX, BX, DX
; -----------------------------------------------------------------------
set_cursor:
    pusha
    
    ; 更新软件跟踪的光标位置
    mov byte [cs:cursor_y], dh
    mov byte [cs:cursor_x], dl
    
    ; 计算光标位置（线性偏移）
    mov al, dh
    mov bl, 80
    mul bl              ; AX = 行 * 80
    mov bl, dl
    add al, bl          ; AL = 行 * 80 + 列
    xor ah, ah          ; AX = 光标位置
    
    ; 设置硬件光标位置
    mov dx, 0x03D4      ; CRTC地址端口
    mov bl, al          ; 保存位置
    mov al, 0x0E        ; 光标位置高字节寄存器
    out dx, al
    inc dx              ; 0x03D5 - CRTC数据端口
    mov al, bh          ; 高字节
    out dx, al
    
    dec dx              ; 0x03D4 - CRTC地址端口
    mov al, 0x0F        ; 光标位置低字节寄存器
    out dx, al
    inc dx              ; 0x03D5 - CRTC数据端口
    mov al, bl          ; 低字节
    out dx, al
    
    popa
    ret

; =================================================================================
; 光标换行函数
; 输入: 无
; 输出: 无
; 破坏: AX, BX, CX, DX, SI, DI
; -----------------------------------------------------------------------
newline:
    pusha
    
    ; 回到行首
    mov byte [cs:cursor_x], 0
    
    ; 移动到下一行
    inc byte [cs:cursor_y]
    
    ; 检查是否超出屏幕底部
    cmp byte [cs:cursor_y], 25
    jb .no_scroll
    
    ; 滚动屏幕（将所有内容上移一行）
    mov ax, 0xB800
    mov ds, ax
    mov es, ax
    mov si, 160         ; 第二行的起始偏移 (80字符 * 2字节)
    mov di, 0           ; 第一行的起始偏移
    mov cx, 80 * 24     ; 24行字符数
    cld
    rep movsw           ; 将所有内容上移一行
    
    ; 清除最后一行
    mov di, 80 * 24 * 2 ; 最后一行的起始偏移
    mov cx, 80
    mov ax, 0x0720      ; 空格字符，黑底白字
    rep stosw
    
    ; 重置光标位置到屏幕底部
    mov byte [cs:cursor_y], 24
    
.no_scroll:
    ; 更新硬件光标位置
    mov dh, [cs:cursor_y]
    mov dl, [cs:cursor_x]
    call set_cursor
    
    popa
    ret

; =================================================================================
; 打印字符函数
; 输入: AL = 字符, AH = 属性
; 输出: 无
; 破坏: AX, BX, CX, DX, DI
; -----------------------------------------------------------------------
print_char:
    pusha
    
    ; 计算光标位置对应的视频内存地址
    mov dh, [cs:cursor_y]
    mov dl, [cs:cursor_x]
    mov bh, 0
    mov bl, dh
    shl bx, 1           ; BX = 行 * 2
    mov di, bx
    shl di, 6           ; DI = 行 * 128
    shl bx, 4           ; BX = 行 * 32
    add di, bx          ; DI = 行 * 160
    mov bl, dl
    xor bh, bh
    shl bx, 1           ; BX = 列 * 2
    add di, bx          ; DI = 行 * 160 + 列 * 2
    
    ; 写入字符和属性
    mov bx, 0xB800
    mov es, bx
    mov [es:di], ax
    
    ; 更新光标位置
    inc byte [cs:cursor_x]
    cmp byte [cs:cursor_x], 80
    jb .no_newline
    call newline
.no_newline:
    
    ; 更新硬件光标位置
    mov dh, [cs:cursor_y]
    mov dl, [cs:cursor_x]
    call set_cursor
    
    popa
    ret

; =================================================================================
; 打印字符串函数
; 输入: DS:SI = 字符串地址, AH = 属性
; 输出: 无
; 破坏: AX, BX, CX, DX, SI, DI
; -----------------------------------------------------------------------
print_string:
    pusha
    
.loop:
    lodsb                ; 加载下一个字符
    test al, al          ; 检查是否为null终止符
    jz .done
    
    ; 打印字符
    push ax
    call print_char
    pop ax
    
    jmp .loop
    
.done:
    popa
    ret

; =================================================================================
; 打印带颜色的消息函数
; 输入: DS:SI = 字符串地址, AH = 属性
; 输出: 无
; 破坏: AX, BX, CX, DX, SI, DI
; -----------------------------------------------------------------------
print_color_msg:
    call print_string
    call newline
    ret

; =================================================================================
; 打印调试信息函数（绿色文本）
; 输入: DS:SI = 字符串地址
; 输出: 无
; 破坏: AX, BX, CX, DX, SI, DI
; -----------------------------------------------------------------------
print_debug_msg:
    push ax
    mov ah, 0x0A        ; 绿底黑字属性
    call print_color_msg
    pop ax
    ret

; =================================================================================
; 打印错误信息函数（红底白字）
; 输入: DS:SI = 字符串地址
; 输出: 无
; 破坏: AX, BX, CX, DX, SI, DI
; -----------------------------------------------------------------------
print_error_msg:
    push ax
    mov ah, 0x4C        ; 红底白字属性
    call print_color_msg
    pop ax
    ret

; =================================================================================
; 打印十六进制数字函数（8位）
; 输入: AL = 数字, AH = 属性
; 输出: 无
; 破坏: AX, BX, CX, DX
; -----------------------------------------------------------------------
print_hex8:
    pusha
    
    ; 保存属性
    mov bh, ah
    
    ; 打印高4位
    mov cl, al
    shr cl, 4
    and cl, 0x0F
    add cl, '0'
    cmp cl, '9'
    jbe .1
    add cl, 7           ; 'A' - '9' - 1
.1:
    mov al, cl
    mov ah, bh
    call print_char
    
    ; 打印低4位
    mov cl, [esp + 16]  ; 从堆栈获取原始AL
    and cl, 0x0F
    add cl, '0'
    cmp cl, '9'
    jbe .2
    add cl, 7           ; 'A' - '9' - 1
.2:
    mov al, cl
    mov ah, bh
    call print_char
    
    popa
    ret

; =================================================================================
; 打印十六进制数字函数（16位）
; 输入: AX = 数字, BH = 属性
; 输出: 无
; 破坏: AX, BX, CX, DX
; -----------------------------------------------------------------------
print_hex16:
    pusha
    
    ; 保存属性
    mov bl, bh
    
    ; 打印高字节
    mov al, ah
    mov ah, bl
    call print_hex8
    
    ; 打印低字节
    mov al, [esp + 16]  ; 从堆栈获取原始AL
    mov ah, bl
    call print_hex8
    
    popa
    ret

; =================================================================================
; 打印十六进制数字函数（32位）
; 输入: EAX = 数字, BH = 属性
; 输出: 无
; 破坏: EAX, EBX, ECX, EDX
; -----------------------------------------------------------------------
print_hex32:
    pusha
    
    ; 保存属性
    mov bl, bh
    
    ; 打印最高字节
    mov al, ah
    mov ah, bl
    call print_hex8
    
    ; 打印次高字节
    mov eax, [esp + 16] ; 从堆栈获取原始EAX
    mov al, al
    mov ah, bl
    call print_hex8
    
    ; 打印次低字节
    mov eax, [esp + 16] ; 从堆栈获取原始EAX
    mov al, ah
    mov ah, bl
    call print_hex8
    
    ; 打印最低字节
    mov eax, [esp + 16] ; 从堆栈获取原始EAX
    mov al, al
    mov ah, bl
    call print_hex8
    
    popa
    ret

; =================================================================================
; 打印寄存器值函数
; 输入: 无
; 输出: 无
; 破坏: AX, BX, CX, DX, SI, DI
; -----------------------------------------------------------------------
dump_registers:
    pusha
    
    ; 打印标题
    mov si, register_dump_msg
    mov ah, 0x1E        ; 黄底蓝字属性
    call print_string
    call newline
    
    ; 打印EAX
    mov si, eax_msg
    mov ah, 0x0F        ; 黑底白字属性
    call print_string
    mov eax, [esp + 28] ; 从堆栈获取原始EAX
    mov bh, 0x0F
    call print_hex32
    call newline
    
    ; 打印EBX
    mov si, ebx_msg
    mov ah, 0x0F
    call print_string
    mov eax, [esp + 24] ; 从堆栈获取原始EBX
    mov bh, 0x0F
    call print_hex32
    call newline
    
    ; 打印ECX
    mov si, ecx_msg
    mov ah, 0x0F
    call print_string
    mov eax, [esp + 20] ; 从堆栈获取原始ECX
    mov bh, 0x0F
    call print_hex32
    call newline
    
    ; 打印EDX
    mov si, edx_msg
    mov ah, 0x0F
    call print_string
    mov eax, [esp + 16] ; 从堆栈获取原始EDX
    mov bh, 0x0F
    call print_hex32
    call newline
    
    ; 打印ESI
    mov si, esi_msg
    mov ah, 0x0F
    call print_string
    mov eax, [esp + 12] ; 从堆栈获取原始ESI
    mov bh, 0x0F
    call print_hex32
    call newline
    
    ; 打印EDI
    mov si, edi_msg
    mov ah, 0x0F
    call print_string
    mov eax, [esp + 8]  ; 从堆栈获取原始EDI
    mov bh, 0x0F
    call print_hex32
    call newline
    
    ; 打印EBP
    mov si, ebp_msg
    mov ah, 0x0F
    call print_string
    mov eax, [esp + 4]  ; 从堆栈获取原始EBP
    mov bh, 0x0F
    call print_hex32
    call newline
    
    ; 打印ESP
    mov si, esp_msg
    mov ah, 0x0F
    call print_string
    mov eax, esp
    add eax, 32         ; 调整堆栈指针
    mov bh, 0x0F
    call print_hex32
    call newline
    
    ; 打印段寄存器
    mov si, cs_msg
    mov ah, 0x0F
    call print_string
    mov ax, cs
    mov bh, 0x0F
    call print_hex16
    call newline
    
    mov si, ds_msg
    mov ah, 0x0F
    call print_string
    mov ax, ds
    mov bh, 0x0F
    call print_hex16
    call newline
    
    mov si, es_msg
    mov ah, 0x0F
    call print_string
    mov ax, es
    mov bh, 0x0F
    call print_hex16
    call newline
    
    mov si, fs_msg
    mov ah, 0x0F
    call print_string
    mov ax, fs
    mov bh, 0x0F
    call print_hex16
    call newline
    
    mov si, gs_msg
    mov ah, 0x0F
    call print_string
    mov ax, gs
    mov bh, 0x0F
    call print_hex16
    call newline
    
    mov si, ss_msg
    mov ah, 0x0F
    call print_string
    mov ax, ss
    mov bh, 0x0F
    call print_hex16
    call newline
    
    popa
    ret

; =================================================================================
; 消息定义
; =================================================================================
section .data
register_dump_msg: db "Register Dump:", 0
eax_msg: db "EAX: ", 0
ebx_msg: db "EBX: ", 0
ecx_msg: db "ECX: ", 0
edx_msg: db "EDX: ", 0
esi_msg: db "ESI: ", 0
edi_msg: db "EDI: ", 0
ebp_msg: db "EBP: ", 0
esp_msg: db "ESP: ", 0
cs_msg: db "CS: ", 0
ds_msg: db "DS: ", 0
es_msg: db "ES: ", 0
fs_msg: db "FS: ", 0
gs_msg: db "GS: ", 0
ss_msg: db "SS: ", 0