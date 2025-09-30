
GLOBAL _cli
GLOBAL _sti
GLOBAL _hlt

GLOBAL picMasterMask
GLOBAL picSlaveMask

GLOBAL _irq00Handler
GLOBAL _irq01Handler
GLOBAL _irq80Handler

GLOBAL _exceptionHandler00
GLOBAL _exceptionHandler06

GLOBAL register_snapshot
GLOBAL register_snapshot_taken

EXTERN irqDispatcher
EXTERN syscallDispatcher
EXTERN exceptionDispatcher
EXTERN getStackBase

SECTION .text

%macro pushState 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popStateButRAX 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
%endmacro

%macro popState 0
	popStateButRAX
	pop rax
%endmacro

%macro irqHandlerMaster 1
	pushState

	mov rdi, %1 ; pass argument to irqDispatcher
	call irqDispatcher

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	iretq
%endmacro

%macro exceptionHandler 1
	cli
	
	mov [exception_register_snapshot + 0x00], rax
	mov [exception_register_snapshot + 0x08], rbx
	mov [exception_register_snapshot + 0x10], rcx
	mov [exception_register_snapshot + 0x18], rdx
	mov [exception_register_snapshot + 0x20], rbp
	mov [exception_register_snapshot + 0x28], rdi
	mov [exception_register_snapshot + 0x30], rsi
	mov [exception_register_snapshot + 0x38], r8
	mov [exception_register_snapshot + 0x40], r9
	mov [exception_register_snapshot + 0x48], r10
	mov [exception_register_snapshot + 0x50], r11
	mov [exception_register_snapshot + 0x58], r12
	mov [exception_register_snapshot + 0x60], r13
	mov [exception_register_snapshot + 0x68], r14
	mov [exception_register_snapshot + 0x70], r15

	mov [exception_register_snapshot + 0x78], rsp ; rsp

	; after the exception, the rip's value that points 
	; to the faulting instruction is, among other things, pushed to the stack
	; it is the last thing pushed in the stack frame

	; https://os.phil-opp.com/cpu-exceptions/#the-interrupt-stack-frame

	mov rax, [rsp + 0x00] ; RIP
	mov [exception_register_snapshot + 0x80], rax

	mov rax, [rsp + 0x10] ; RFLAGS
	mov [exception_register_snapshot + 0x88], rax

	mov rdi, %1 ; pass argument to exceptionDispatcher
	mov rsi, exception_register_snapshot ;pass current register values to exceptionDispatcher
	
	call exceptionDispatcher

	call getStackBase ; reset the stack
	mov [rsp + 0x18], rax

	mov QWORD [rsp], USERLAND ; set return address to userland

	sti
	iretq ; will pop USERLAND and jmp to it
%endmacro

_hlt:
	sti
	hlt
	ret

_cli:
	cli
	ret

_sti:
	sti
	ret

picMasterMask:
	push rbp     ; Stack frame
	mov rbp, rsp
	mov rax, rdi

	out 0x21, al ; https://wiki.osdev.org/8259_PIC#Masking

	mov rsp, rbp
	pop rbp
	ret

picSlaveMask:
	push rbp
	mov rbp, rsp
	mov rax, rdi

	out 0xA1, al ; https://wiki.osdev.org/8259_PIC#Masking

	mov rsp, rbp
	pop rbp
	ret

; 8254 Timer (Timer Tick)
_irq00Handler:
	irqHandlerMaster 0

; Keyboard
_irq01Handler:
	pushfq
	pushState

	mov rdi, 1 ; pass argument to irqDispatcher
	call irqDispatcher

	cmp rax, REGISTER_SNAPSHOT_KEY_SCANCODE ; F12 KEY SCANCODE
	jne .skip

	popState  ; restore register values
	pushState ; preserve stack frame

	mov [register_snapshot + 0x08 * 0x00], rax
	mov [register_snapshot + 0x08 * 0x01], rbx
	mov [register_snapshot + 0x08 * 0x02], rcx
	mov [register_snapshot + 0x08 * 0x03], rdx
	mov [register_snapshot + 0x08 * 0x04], rbp
	mov [register_snapshot + 0x08 * 0x05], rdi
	mov [register_snapshot + 0x08 * 0x06], rsi
	mov [register_snapshot + 0x08 * 0x07], r8
	mov [register_snapshot + 0x08 * 0x08], r9
	mov [register_snapshot + 0x08 * 0x09], r10
	mov [register_snapshot + 0x08 * 0x0A], r11
	mov [register_snapshot + 0x08 * 0x0B], r12
	mov [register_snapshot + 0x08 * 0x0C], r13
	mov [register_snapshot + 0x08 * 0x0D], r14
	mov [register_snapshot + 0x08 * 0x0E], r15

	mov [register_snapshot + 0x08 * 0x0F], rsp ; rsp

	mov rax, [ rsp + 0x08 * 16 ] ; get the return address
	mov [register_snapshot + 0x08 * 0x10], rax ; rip

	mov rax, [ rsp + 0x08 * 15 ] ; get the rflags
	mov [register_snapshot + 0x08 * 0x11], rax ; rflags

	mov byte [register_snapshot_taken], 0x01

	.skip:
	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	add rsp, 0x08 ; remove rflags from the stack

	iretq

; System Call
; Not using the %irqHandlerMaster macro because it needs to pass the stack pointer to the syscall
_irq80Handler:
	pushState

	mov rdi, rsp ; pass REGISTERS (stack) to irqDispatcher, see: `pushState` two lines above
	call syscallDispatcher

	mov rbx, rax

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	mov rax, rbx

	popStateButRAX
	add rsp, 8 ; skip the error code pushed by irqDispatcher
	iretq

; Zero Division Exception
_exceptionHandler00:
	exceptionHandler 0

; Invalid Opcode Exception
_exceptionHandler06:
	exceptionHandler 6

section .bss
	exception_register_snapshot resq 18
	register_snapshot resq 18
	register_snapshot_taken resb 1

section .rodata
	REGISTER_SNAPSHOT_KEY_SCANCODE equ 0x58 ; F12 KEY SCANCODE
	USERLAND equ 0x400000 ; userland (shell module address)