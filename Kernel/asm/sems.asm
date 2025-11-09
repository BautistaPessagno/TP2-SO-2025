GLOBAL semLock
GLOBAL semUnlock

SECTION .text

; https://wiki.osdev.org/Spinlock

semLock:
    mov al, 1
    xchg al, BYTE [rdi]
    cmp al, 0
    jne semLock
    ret

semUnlock:
    mov BYTE [rdi], 0
    ret