#include <fonts.h>
#include <interrupts.h>
#include <syscallDispatcher.h>
#include <keyboard.h>

const static char * register_names[] = {
	"rax", "rbx", "rcx", "rdx", "rbp", "rdi", "rsi", "r8 ", "r9 ", "r10", "r11", "r12", "r13", "r14", "r15", "rsp", "rip", "rflags"
};

const static int registers_amount = sizeof(register_names) / sizeof(*register_names);

#define ZERO_EXCEPTION_ID 0
#define INVALID_OPCODE_ID 6

static void zero_division(uint64_t * registers, int errorCode);
static void invalid_opcode(uint64_t * registers, int errorCode);

void printExceptionData(uint64_t * registers, int errorCode);

void exceptionDispatcher(int exception, uint64_t * registers) {
	clear();
	switch(exception) {
		case ZERO_EXCEPTION_ID:
			return zero_division(registers, exception);
		case INVALID_OPCODE_ID:
			return invalid_opcode(registers, exception);
		default:
			return ; // returns to the asm exceptionHandler which will return to the shell
	}
}

static void zero_division(uint64_t * registers, int errorCode) {
	setTextColor(0x00FF0000);
	setFontSize(3); print("Division exception\n"); setFontSize(2);
	print("Arqui Screen of Death\n");
	printExceptionData(registers, errorCode);
}

static void invalid_opcode(uint64_t * registers, int errorCode) {
	setTextColor(0x00FF6600);
	setFontSize(3); print("Invalid Opcode Exception\n"); setFontSize(2);
	print("Arqui screen of death\n");
	printExceptionData(registers, errorCode);
}


void printExceptionData(uint64_t * registers, int errorCode) {
	print("Exception (# "); printDec(errorCode); print(") triggered\n");
	print("Current registers values are\n");
	
	for (int i = 0; i < registers_amount; i++) {
		print(register_names[i]); print(": "); printHex((uint64_t) registers[i]); newLine();
	}

	setTextColor(0x00FFFFFF);

	print("Press r to go back to Shell");

	char a;
	// getKeyboardCharacter calls _hlt which triggers _sti
	// so non-keyboard interrupts are disabled until the user confirms

	picMasterMask(KEYBOARD_PIC_MASTER);
	picSlaveMask(NO_INTERRUPTS);
	while ((a = getKeyboardCharacter(0)) != 'r') {}
	picMasterMask(KEYBOARD_PIC_MASTER & TIMER_PIC_MASTER);
	picSlaveMask(NO_INTERRUPTS);

	return ;
}