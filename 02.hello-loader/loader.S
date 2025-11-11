bits 16
org 0x8000

    mov ax, 0xb800          ;显存位置
    mov es, ax

    mov byte[es: 0x00], 'L'
    mov byte[es: 0x01], 0x07
    mov byte[es: 0x02], 'O'
    mov byte[es: 0x03], 0x06
    mov byte[es: 0x04], 'A'
    mov byte[es: 0x05], 0x07
    mov byte[es: 0x06], 'D'
    mov byte[es: 0x07], 0x06
    mov byte[es: 0x08], 'E'
    mov byte[es: 0x09], 0x07
    mov byte[es: 0x0a], 'R'
    mov byte[es: 0x0b], 0x06
    jmp $