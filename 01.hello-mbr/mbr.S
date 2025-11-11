bits 16
org 0x7C00

start:
    call clear_screen     ; 调用清屏函数
    mov si, msg
    call print_string
    jmp $                 ; 无限循环

; ------------------------------
; 清屏 + 重置光标函数
; ------------------------------
clear_screen:
    ; 清屏
    mov ax, 0x0600        ; AH=06h 滚屏，AL=0 清屏
    mov bh, 0x07          ; 属性（白字黑底）
    mov cx, 0x0000        ; 左上角 (0,0)
    mov dx, 0x184F        ; 右下角 (24,79)
    int 0x10

    ; 重置光标到左上角
    mov ah, 0x02          ; 设置光标位置
    mov bh, 0x00          ; 显示页0
    mov dh, 0x00          ; 行=0
    mov dl, 0x00          ; 列=0
    int 0x10
    ret

; ------------------------------
; 打印字符串函数
; DS:SI 指向字符串，以 0 结尾
; ------------------------------
print_string:
    lodsb                 ; 取下一个字符到 AL
    or al, al             ; 判断是否为 0
    jz .done
    mov ah, 0x0E          ; BIOS 打印字符
    mov bh, 0x00          ; 显示页0
    int 0x10
    jmp print_string
.done:
    ret

; ------------------------------
; 数据区
; ------------------------------
msg db "hello mbr", 0

; ------------------------------
; 填充 & MBR 签名
; ------------------------------
times 510 - ($ - $$) db 0
dw 0xAA55