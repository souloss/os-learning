bits 16
org 0x7c00

%include "../common/bootmacros.inc"

loader_base_addr        equ 0x8000      ; loader 所处的段地址
loader_sectors          equ 8           ; 需要读取的扇区数

        ; 初始化寄存器
        mov ax, cs
        mov ds, ax
        mov ss, ax
        mov fs, ax
        ; es 段赋0
        mov ax, 0
        mov es, ax
        ; 将 0x7c00 以下的区域作为栈
        mov sp, 0x7c00
        ; 显存段位置
        mov ax, 0xb800            
        mov gs, ax

        ; 清空屏幕
        call cls

        ; 输出
        mov bx, hello_msg
        call print ; This will be written after the BIOS messages

        ; 获取内存信息
        call get_memory_map

        ;读loader
        mov dh,  loader_sectors
        mov dl,  0x80
        mov bx,  loader_base_addr       
        call disk_load
        jmp 0:loader_base_addr          ;跳转至 loader，也就是 0x9000

hello_msg: db "HELLO, MBR!", 0

cls:
        mov ax, 0x0600       ; ah=06h(滚动窗口功能), al=00h(清空整个窗口)
        mov bx, 0x0700       ; bh=07h(空白行的显示属性: 黑底白字)
        mov cx, 0            ; ch=00h(左上角行号=0), cl=00h(左上角列号=0)
        mov dx, 184fh        ; dh=18h(右下角行号=24), dl=4fh(右下角列号=79)
        int 10h              ; 调用bios中断
        ret

; load 'dh' sectors from drive 'dl' into es:bx
disk_load:
        pusha
        ; reading from disk requires setting specific values in all registers
        ; so we will overwrite our input parameters from 'dx'. let's save it
        ; to the stack for later use.
        push dx

        mov ah, 0x02 ; ah <- int 0x13 function. 0x02 = 'read'
        mov al, dh   ; al <- number of sectors to read (0x01 .. 0x80)
        mov cl, 0x02 ; cl <- sector (0x01 .. 0x11)
                        ; 0x01 is our boot sector, 0x02 is the first 'available' sector
        mov ch, 0x00 ; ch <- cylinder (0x0 .. 0x3ff, upper 2 bits in 'cl')
        ; dl <- drive number. our caller sets it as a parameter and gets it from bios
        ; (0 = floppy, 1 = floppy2, 0x80 = hdd, 0x81 = hdd2)
        mov dh, 0x00 ; dh <- head number (0x0 .. 0xf)

        ; [es:bx] <- pointer to buffer where the data will be stored
        ; caller sets it up for us, and it is actually the standard location for int 13h
        int 0x13      ; bios interrupt
        jc disk_error ; if error (stored in the carry bit)

        pop dx
        cmp al, dh    ; bios also sets 'al' to the # of sectors read. compare it.
        jne sectors_error
        popa
        ret


disk_error:
        mov bx, disk_error_msg
        call print
        call print_nl
        mov dh, ah ; ah = error code, dl = disk drive that dropped the error
        call print_hex ; check out the code at http://stanislavs.org/helppc/int_13-1.html
        jmp disk_loop

sectors_error:
        mov bx, sectors_error_msg
        call print

disk_loop:
        jmp $

disk_error_msg: db "disk read error", 0
sectors_error_msg: db "incorrect number of sectors read", 0

print:
        pusha
print_start:
        mov al, [bx]    ; 'bx' is the base address for the string
        cmp al, 0 
        je done

        ; the part where we print with the bios help
        mov ah, 0x0e
        int 0x10        ; 'al' already contains the char

        ; increment pointer and do next loop
        add bx, 1
        jmp print_start

done:
        popa
        ret

print_nl:
        pusha

        mov ah, 0x0e
        mov al, 0x0a ; newline 
        int 0x10
        mov al, 0x0d ; carriage return
        int 0x10
    
        popa
        ret

; receiving the data in 'dx'
; For the examples we'll assume that we're called with dx=0x1234
print_hex:
    pusha

    mov cx, 0 ; our index variable

; Strategy: get the last char of 'dx', then convert to ASCII
; Numeric ASCII values: '0' (ASCII 0x30) to '9' (0x39), so just add 0x30 to byte N.
; For alphabetic characters A-F: 'A' (ASCII 0x41) to 'F' (0x46) we'll add 0x40
; Then, move the ASCII byte to the correct position on the resulting string
hex_loop:
    cmp cx, 4 ; loop 4 times
    je end
    
    ; 1. convert last char of 'dx' to ascii
    mov ax, dx ; we will use 'ax' as our working register
    and ax, 0x000f ; 0x1234 -> 0x0004 by masking first three to zeros
    add al, 0x30 ; add 0x30 to N to convert it to ASCII "N"
    cmp al, 0x39 ; if > 9, add extra 8 to represent 'A' to 'F'
    jle step2
    add al, 7 ; 'A' is ASCII 65 instead of 58, so 65-58=7

step2:
    ; 2. get the correct position of the string to place our ASCII char
    ; bx <- base address + string length - index of char
    mov bx, hex_out + 5 ; base + length
    sub bx, cx  ; our index variable
    mov [bx], al ; copy the ASCII char on 'al' to the position pointed by 'bx'
    ror dx, 4 ; 0x1234 -> 0x4123 -> 0x3412 -> 0x2341 -> 0x1234

    ; increment index and loop
    add cx, 1
    jmp hex_loop

end:
    ; prepare the parameter and call the function
    ; remember that print receives parameters in 'bx'
    mov bx, hex_out
    call print

    popa
    ret

get_memory_map:
    mov di, E820_BUFFER
    xor ebx, ebx            ; EBX=0 for first call
.get_next:
    mov eax, 0xE820
    mov ecx, 24             ; size of e820 entry
    mov edx, E820_SIGNATURE 
    int 0x15
    jc .done                ; CF=1 -> done
    cmp eax, E820_SIGNATURE
    jne .done
    ; 准备写下一条目标
    add di, 24
    test ebx, ebx
    jnz .get_next
.done:
    ret


hex_out:
    db '0x0000',0 ; reserve memory for our new string

        ; 签名
        times 510 - ($ - $$) db 0
        db 0x55, 0xaa

