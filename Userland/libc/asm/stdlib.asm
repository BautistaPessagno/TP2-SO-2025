GLOBAL sys_write
GLOBAL sys_read

section .text

sys_write:
    push rbp
    mov rbp, rsp
    mov rax, 0x04
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_read:
    push rbp
    mov rbp, rsp
    mov rax, 0x03
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

