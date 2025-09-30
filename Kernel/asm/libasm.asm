GLOBAL cpuVendor
GLOBAL getKeyboardBuffer

GLOBAL getSecond
GLOBAL getMinute
GLOBAL getHour

GLOBAL setPITMode
GLOBAL setPITFrequency
GLOBAL setSpeaker

GLOBAL getRegisterSnapshot

EXTERN register_snapshot
EXTERN register_snapshot_taken

section .text

getKeyboardBuffer:
	push rbp
	mov rbp, rsp

	in al, 60h

	mov rsp, rbp
	pop rbp

	ret


getSecond:
	push rbp
	mov rbp, rsp

	mov al, 0
	out 70h, al
	in al, 71h

	mov rsp, rbp
	pop rbp

	ret


getMinute:
	push rbp
	mov rbp, rsp

	mov al, 2
	out 70h, al
	in al, 71h

	mov rsp, rbp
	pop rbp

	ret


getHour:
	push rbp
	mov rbp, rsp

	mov al, 4
	out 70h, al
	in al, 71h

	mov rsp, rbp
	pop rbp

	ret


setPITMode:
	push rbp
	mov rbp, rsp
	
	mov rax, rdi
	out 0x43, al

	mov rsp, rbp
	pop rbp

	ret


setPITFrequency:
	push rbp
	mov rbp, rsp

	mov rax, rdi
	out 0x42, al
	mov al, ah
	out 0x42, al

	mov rsp, rbp
	pop rbp
	ret


setSpeaker:
	push rbp
	mov rbp, rsp

	cmp rdi, 0
	je .off

	.on:
	in al, 0x61
	or al, 0x03
	out 0x61, al
	jmp .end

	.off:
	in al, 0x61
	and al, 0xFC
	out 0x61, al

	.end:
	mov rsp, rbp
	pop rbp
	ret
