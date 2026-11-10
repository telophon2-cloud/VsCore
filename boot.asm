[org 0x7c00]
KERNEL_OFFSET equ 0x1000

    ; Жесткая инициализация сегментов памяти
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov bp, 0x7c00
    mov sp, bp

    ; BIOS передает номер диска в DL, сохраняем его
    mov [BOOT_DRIVE], dl

    ; Сброс дисковой подсистемы
    xor ah, ah
    mov dl, [BOOT_DRIVE]
    int 0x13

    ; Загрузка ядра (читаем 40 секторов со 2-го сектора диска)
    mov bx, KERNEL_OFFSET
    mov dh, 40
    mov ah, 0x02
    mov al, dh
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    ; Переход в 32-битный защищенный режим
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm

disk_error:
    jmp $

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Передаем управление коду ядра по адресу 0x1000
    jmp CODE_SEG:KERNEL_OFFSET

gdt_start:
    dd 0x0, 0x0
gdt_code:
    dw 0xffff, 0x0
    db 0x0, 0x9a, 0xcf, 0x0
gdt_data:
    dw 0xffff, 0x0
    db 0x0, 0x92, 0xcf, 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

BOOT_DRIVE db 0
times 510-($-$$) db 0
dw 0xaa55
