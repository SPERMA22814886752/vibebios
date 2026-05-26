[bits 32]
[org 0x7C00]

VGA_LFB     equ 0xE0000000  ; Адрес из VIBEBIOS
SCREEN_W    equ 800
COLOR_WHITE equ 63
COLOR_BG    equ 10          ; Тёмно-синий

KBD_DATA_PORT   equ 0x60
KBD_STATUS_PORT equ 0x64

start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x7C00

    call clear_screen
    
    ; Приветствие
    mov esi, msg_welcome
    mov ebx, 20
    mov edx, 20
    call draw_string

    ; Промпт шелла
    mov esi, msg_prompt
    mov ebx, 20
    mov edx, 50
    call draw_string

shell_loop:
    in al, KBD_STATUS_PORT
    test al, 1
    jz shell_loop

    in al, KBD_DATA_PORT
    test al, 0x80
    jnz shell_loop

    cmp al, 0x1C            ; Enter
    je .on_enter
    cmp al, 0x01            ; Esc
    je .on_esc
    jmp shell_loop

.on_enter:
    mov esi, msg_err
    mov ebx, 20
    mov edx, 80
    call draw_string
    jmp shell_loop

.on_esc:
    call clear_screen
    mov esi, msg_reboot
    mov ebx, 20
    mov edx, 20
    call draw_string
    
    mov al, 0xFE
    out 0x64, al
    jmp shell_loop

; --- ВЫВОД ---

clear_screen:
    mov edi, VGA_LFB
    mov ecx, SCREEN_W * 600
    mov al, COLOR_BG
    rep stosb
    ret

; Поиск символа в микро-шрифте
find_char:
    push ecx
    mov edi, font_index
    mov ecx, font_index_end - font_index
    repne scasb
    jne .not_found
    
    sub edi, font_index + 1
    mov eax, edi
    shl eax, 3              ; индекс * 8
    mov esi, font_data
    add esi, eax
    pop ecx
    ret
.not_found:
    mov esi, font_unknown
    pop ecx
    ret

; Рисование буквы 8x8 (масштаб 2x2)
draw_char:
    pushad
    call find_char

    mov eax, edx
    mov ecx, SCREEN_W
    mul ecx
    add eax, ebx
    mov edi, VGA_LFB
    add edi, eax

    mov ecx, 8
.row_loop:
    push edi
    lodsb
    mov dl, al
    
    mov ch, 8
.col_loop:
    test dl, 0x80
    jz .skip_pixel
    
    mov byte [edi], COLOR_WHITE
    mov byte [edi+1], COLOR_WHITE
    mov byte [edi+SCREEN_W], COLOR_WHITE
    mov byte [edi+SCREEN_W+1], COLOR_WHITE

.skip_pixel:
    add edi, 2
    shl dl, 1
    dec ch
    jnz .col_loop

    pop edi
    add edi, SCREEN_W * 2
    loop .row_loop

    popad
    ret

draw_string:
    pushad
.loop:
    lodsb
    or al, al
    jz .done
    call draw_char
    add ebx, 18
    jmp .loop
.done:
    popad
    ret

; --- СТРОКИ (Оптимизированные по длине) ---
msg_welcome db "Welcome to WilixOS", 0
msg_prompt  db "wilix:vibebios$ ", 0  ; Заменили @ на двоеточие
msg_err     db "No cmd", 0            ; Сократили ошибку, сэкономив байты
msg_reboot  db "Reboot...", 0

; --- УЛЬТРА-СЖАТЫЙ ШРИФТ (Выкинули всё лишнее) ---
align 4
font_index:
    db " WilexoOS:b$Ncmd"     ; Только то, что реально выводится
font_index_end:

font_unknown:
    db 0xFF,0x81,0x81,0x81,0x81,0x81,0xFF,0x00

font_data:
    db 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ; Пробел
    db 0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00 ; 'W'
    db 0x18,0x00,0x18,0x18,0x18,0x18,0x00,0x00 ; 'i'
    db 0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00 ; 'l'
    db 0x3C,0x66,0x7E,0x60,0x66,0x3C,0x00,0x00 ; 'e'
    db 0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00 ; 'x'
    db 0x3C,0x66,0x66,0x66,0x66,0x3C,0x00,0x00 ; 'o'
    db 0x3C,0x66,0x66,0x66,0x66,0x3C,0x00,0x00 ; 'O'
    db 0x3C,0x60,0x3C,0x06,0x06,0x3C,0x00,0x00 ; 'S'
    db 0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00 ; ':'
    db 0x60,0x60,0x7C,0x66,0x66,0x7C,0x00,0x00 ; 'b'
    db 0x1C,0x3E,0x1C,0x1C,0x3E,0x1C,0x1C,0x00 ; '$'
    db 0x66,0x7E,0x76,0x6E,0x66,0x66,0x00,0x00 ; 'N'
    db 0x3C,0x66,0x60,0x60,0x66,0x3C,0x00,0x00 ; 'c'
    db 0x66,0x6E,0x76,0x66,0x66,0x66,0x00,0x00 ; 'm'
    db 0x7C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00 ; 'd'

; --- СИГНАТУРА ЗАГРУЗКИ ---
times 510-($-$$) db 0
dw 0xAA55