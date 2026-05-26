extern bios_main

; === 32-БИТНОЕ ЯДРО (0xFFFE0000) ===
section .text
[bits 32]
start32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    call bios_main

hang:
    cli
    hlt
    jmp hang


; === 16-БИТНЫЙ БУТБЛОК (0xFFFFFF00) ===
section .boot
[bits 16]
align 4
gdt_start:
    dd 0, 0
gdt_code:
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xCF, 0x00
gdt_data:
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

_start_real:
    cli
    cld

    ; Вычисляем адрес GDT вручную, обходя баги линковщика
    mov bx, (gdt_descriptor - $$) + 0xFF00
    db 0x66
    lgdt [cs:bx]

    ; Включаем Protected Mode
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; Прыжок в 32 бита
    jmp dword 0x08:start32

; === ВЕКТОР ПЕРЕЗАГРУЗКИ ===
; Добиваем нулями ровно до адреса 0xFFFFFFF0 (240-й байт секции)
times 240 - ($ - $$) db 0

global reset_vector
reset_vector:
    jmp _start_real

; Добиваем нулями до самого конца 4 ГБ (255-й байт)
times 255 - ($ - $$) db 0
; Последний байт (NOP). Заставляет утилиту objcopy сделать файл ровно 128 КБ!
db 0x90