#include <keyboard.h>
#include <fonts.h>
#include <interrupts.h>
#include <cursor.h>
#include <stddef.h>

#define BUFFER_SIZE 1024

#define BUFFER_IS_FULL ((to_write - to_read) % BUFFER_SIZE == BUFFER_SIZE - 1)

#define IS_ALPHA(c) ('a' <= (c) && (c) <= 'z') 
#define TO_UPPER(c) (IS_ALPHA(c) ? ((c) - 'a' + 'A') : (c))

#define IS_KEYCODE(c) (c >= ESCAPE_KEY && c <= F12_KEY)
#define IS_PRINTABLE(c) (\
    IS_KEYCODE((c)) && (\
    ((c) >= 0x02 && (c) <= 0x0D) || /* 1,2,3,4,5,6,7,8,9,0,-,= */ \
    ((c) >= 0x0F && (c) <= 0x1C) || /* TAB,q,w,e,r,t,y,u,i,o,p,[,],RET */ \
    ((c) >= 0x1E && (c) <= 0x29) || /* a,s,d,f,g,h,j,k,l,;,',` */ \
    ((c) >= 0x2B && (c) <= 0x35) || /* \,z,x,c,v,b,n,m,,,.,/ */ \
    ((c) == 0x37) || /* * */ \
    ((c) == 0x39) || /* space */ \
    ((SHIFT_KEY_PRESSED) && (c) >= 0x47 && (c) <= 0x53) || /* home,up,pageup,-,left,5,right,+,end,down,pagedown,insert,delete */ \
    ((c) == 0x4A || (c) == 0x4E) || /* -,+ */ \
    ((c)) == 0x56 \
    ))

#define IS_SPECIAL_KEY(c) (IS_KEYCODE(c) && !IS_PRINTABLE(c))

#define INC_MOD(x, m) ((x) = ((x) + 1) % (m))
#define SUB_MOD(a, b, m) ((a) - (b) < 0 ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

static uint8_t SHIFT_KEY_PRESSED, CAPS_LOCK_KEY_PRESSED, CONTROL_KEY_PRESSED;
static int8_t buffer[BUFFER_SIZE];
static uint16_t to_write = 0, to_read = 0;
uint8_t keyboard_options = 0;

typedef struct {
    uint8_t registered_from_kernel;
    SpecialKeyHandler fn;
} RegisteredKeys;

static RegisteredKeys KeyFnMap[ F12_KEY - ESCAPE_KEY + 1 ] = {0};

// QEMU source https://github.com/qemu/qemu/blob/master/pc-bios/keymaps/en-us
// http://flint.cs.yale.edu/feng/cos/resources/BIOS/Resources/assembly/makecodes.html
// Array of scancodes to ASCII - Shift-Modified-ASCII
// Note: this is NOT the complete QEMU scancode set, but it does include all printable characters and most control keys
static const uint8_t scancodeMap[][2] = {
    /* 0x00 */ { 0, 0 },
    /* 0x01 */ { ESCAPE_KEY, ESCAPE_KEY },
    /* 0x02 */ { '1', '!' },
    /* 0x03 */ { '2', '@' },
    /* 0x04 */ { '3', '#' },
    /* 0x05 */ { '4', '$' },
    /* 0x06 */ { '5', '%' },
    /* 0x07 */ { '6', '^' },
    /* 0x08 */ { '7', '&' },
    /* 0x09 */ { '8', '*' },
    /* 0x0A */ { '9', '(' },
    /* 0x0B */ { '0', ')' },
    /* 0x0C */ { '-', '_' },
    /* 0x0D */ { '=', '+' },
    /* 0x0E */ { BACKSPACE_KEY, BACKSPACE_KEY },
    /* 0x0F */ { TABULATOR_KEY, TABULATOR_KEY },
    /* 0x10 */ { 'q', 'Q' },
    /* 0x11 */ { 'w', 'W' },
    /* 0x12 */ { 'e', 'E' },
    /* 0x13 */ { 'r', 'R' },
    /* 0x14 */ { 't', 'T' },
    /* 0x15 */ { 'y', 'Y' },
    /* 0x16 */ { 'u', 'U' },
    /* 0x17 */ { 'i', 'I' },
    /* 0x18 */ { 'o', 'O' },
    /* 0x19 */ { 'p', 'P' },
    /* 0x1A */ { '[', '{' },
    /* 0x1B */ { ']', '}' },
    /* 0x1C */ { RETURN_KEY, RETURN_KEY },
    /* 0x1D */ { CONTROL_KEY_L, CONTROL_KEY_L },
    /* 0x1E */ { 'a', 'A' },
    /* 0x1F */ { 's', 'S' },
    /* 0x20 */ { 'd', 'D' },
    /* 0x21 */ { 'f', 'F' },
    /* 0x22 */ { 'g', 'G' },
    /* 0x23 */ { 'h', 'H' },
    /* 0x24 */ { 'j', 'J' },
    /* 0x25 */ { 'k', 'K' },
    /* 0x26 */ { 'l', 'L' },
    /* 0x27 */ { ';', ':' },
    /* 0x28 */ { '\'', '"' },
    /* 0x29 */ { '`', '~' },
    /* 0x2A */ { SHIFT_KEY_L, SHIFT_KEY_L },
    /* 0x2B */ { '\\', '|' },
    /* 0x2C */ { 'z', 'Z' },
    /* 0x2D */ { 'x', 'X' },
    /* 0x2E */ { 'c', 'C' },
    /* 0x2F */ { 'v', 'V' },
    /* 0x30 */ { 'b', 'B' },
    /* 0x31 */ { 'n', 'N' },
    /* 0x32 */ { 'm', 'M' },
    /* 0x33 */ { ',', '<' },
    /* 0x34 */ { '.', '>' },
    /* 0x35 */ { '/', '?' },
    /* 0x36 */ { SHIFT_KEY_R, SHIFT_KEY_R },
    /* 0x37 */ { '*', '*' },
    /* 0x38 */ { ALT_KEY_L, META_L_KEY }, // Left Alt
    /* 0x39 */ { ' ', ' ' },
    /* 0x3A */ { CAPS_LOCK_KEY, CAPS_LOCK_KEY }, // Caps Lock
    /* 0x3B */ { F1_KEY, F1_KEY },
    /* 0x3C */ { F2_KEY, F2_KEY },
    /* 0x3D */ { F3_KEY, F3_KEY },
    /* 0x3E */ { F4_KEY, F4_KEY },
    /* 0x3F */ { F5_KEY, F5_KEY },
    /* 0x40 */ { F6_KEY, F6_KEY },
    /* 0x41 */ { F7_KEY, F7_KEY },
    /* 0x42 */ { F8_KEY, F8_KEY },
    /* 0x43 */ { F9_KEY, F9_KEY },
    /* 0x44 */ { F10_KEY, F10_KEY },
    /* 0x45 */ { NUM_LOCK_KEY, NUM_LOCK_KEY },
    /* 0x46 */ { SCROLL_LOCK_KEY, SCROLL_LOCK_KEY },
    /* 0x47 */ { KP_HOME_KEY, '7' },
    /* 0x48 */ { KP_UP_KEY, '8' },
    /* 0x49 */ { KP_PAGE_UP_KEY, '9' },
    /* 0x4A */ { '-', '-' },
    /* 0x4B */ { KP_LEFT_KEY, '4' },
    /* 0x4C */ { KP_BEGIN_KEY, '5' },
    /* 0x4D */ { KP_RIGHT_KEY, '6' },
    /* 0x4E */ { '+', '+' },
    /* 0x4F */ { KP_END_KEY, '1' },
    /* 0x50 */ { KP_DOWN_KEY, '2'}, 
    /* 0x51 */ { KP_PAGE_DOWN_KEY, '3' },
    /* 0x52 */ { KP_INSERT_KEY, '0' },
    /* 0x53 */ { KP_DELETE_KEY, '.' },
    /* 0x54 */ { 0, 0 }, // 0x54
    /* 0x55 */ { 0, 0 }, // 0x55
    /* 0x56 */ { '-', '-' },
    /* 0x57 */ { F11_KEY, F11_KEY },
    /* 0x58 */ { F12_KEY, F12_KEY },
};

void restoreKeyFnMapNonKernel(SpecialKeyHandler * map) {
    for(uint8_t i = ESCAPE_KEY; i < F12_KEY; i++){
        if (KeyFnMap[i].registered_from_kernel == 0) {
            KeyFnMap[i].fn = map[i];
        }
    }
}

void clearKeyFnMapNonKernel(SpecialKeyHandler * map) {
    for(uint8_t i = ESCAPE_KEY; i < F12_KEY; i++){
        if (KeyFnMap[i].registered_from_kernel == 0) {
            map[i] = KeyFnMap[i].fn;
            KeyFnMap[i].fn = NULL;
        }
    }
}

uint8_t registerSpecialKey(enum KEYS scancode, SpecialKeyHandler fn, uint8_t registeredFromKernel) {
    if (IS_KEYCODE(scancode) && ((registeredFromKernel != 0 || (registeredFromKernel == 0 && KeyFnMap[scancode].fn == NULL)))) {
        KeyFnMap[scancode].fn = fn;
        KeyFnMap[scancode].registered_from_kernel = registeredFromKernel;
        return 1;
    }

    return 0;
}

static uint8_t isReleased(uint8_t scancode) {
    return scancode & 0x80;
}

static uint8_t isPressed(uint8_t scancode) {
    return !(isReleased(scancode));
}

// static uint8_t isShift(uint8_t scancode){
//     uint8_t aux = scancode & 0x7F;
//     return aux == SHIFT_KEY_L || aux == SHIFT_KEY_R;
// }

// static uint8_t isCapsLock(uint8_t scancode){
//     return (scancode & 0x7F) == CAPS_LOCK_KEY;
// }

// static uint8_t isControl(uint8_t scancode){
//     return (scancode & 0x7F) == CONTROL_KEY_L;
// }

static uint8_t makeCode(uint8_t scancode) {
    return scancode & 0x7F;
}

void addCharToBuffer(int8_t ascii, uint8_t showOutput) {
    if (ascii != TABULATOR_CHAR) {
        buffer[to_write] = ascii;
        INC_MOD(to_write, BUFFER_SIZE);
        if (showOutput)
            putChar(ascii);
        return ;
    }

    do {
        addCharToBuffer(' ', showOutput);
    } while( !BUFFER_IS_FULL && getXBufferPosition() % (TAB_SIZE * DEFAULT_GLYPH_SIZE_X * getFontSize()) != 0);
}

uint16_t clearBuffer() {
    uint16_t aux = SUB_MOD(to_write, to_read, BUFFER_SIZE);
    if (aux == 0) return 0;
    DEC_MOD(to_write, BUFFER_SIZE);
    clearPreviousCharacter();
    return aux;
}

// Halts until any key is pressed or \n is entered, depending on keyboard_options (AWAIT_RETURN_KEY)
// This function always sets the MODIFY_BUFFER option, so keys can be consumed
int8_t getKeyboardCharacter(enum KEYBOARD_OPTIONS ops) {
    keyboard_options = ops | MODIFY_BUFFER;

    while(
        to_write == to_read || // always get at least one char from the buffer if empty
        (   (keyboard_options & AWAIT_RETURN_KEY) && // wait for \n or EOF to be entered by the user
            !(buffer[SUB_MOD(to_write, 1, BUFFER_SIZE)] == NEW_LINE_CHAR || buffer[SUB_MOD(to_write, 1, BUFFER_SIZE)] == EOF)
        )) _hlt();

    keyboard_options = 0;
    int8_t aux = buffer[to_read];
    INC_MOD(to_read, BUFFER_SIZE);
    return aux;
}

uint8_t keyboardHandler(){
    uint8_t scancode = getKeyboardBuffer();
    uint8_t is_pressed = isPressed(scancode);

    if(BUFFER_IS_FULL){
        to_read = to_write = 0;
        return scancode; // do not write to buffer anymore, subsequent keys are not processed into the buffer
    }
    
    switch (makeCode(scancode)) {
        case SHIFT_KEY_L:
        case SHIFT_KEY_R:
            SHIFT_KEY_PRESSED = is_pressed;
            break;
        case CONTROL_KEY_L:
            CONTROL_KEY_PRESSED = is_pressed;
            break;
        case CAPS_LOCK_KEY:
            if (is_pressed)
                CAPS_LOCK_KEY_PRESSED = !CAPS_LOCK_KEY_PRESSED;
            break;

        return scancode;
    }
    
    if (! (is_pressed && IS_KEYCODE(scancode)) ) return scancode; // ignore break or unsupported scancodes
    
    if ((keyboard_options & MODIFY_BUFFER) != 0) {
        int8_t c = scancodeMap[scancode][SHIFT_KEY_PRESSED];

        if (CAPS_LOCK_KEY_PRESSED == 1) {
            c = TO_UPPER(c);
        }

        if (IS_PRINTABLE(scancode)) {
            if(c == RETURN_KEY){
                c = NEW_LINE_CHAR;
                // Handle \n on the keyboard interrupt handler, to avoid the possibility of triggering multiple \n inputs continously on the same sys_read
                if ( (to_write != to_read) && buffer[SUB_MOD(to_write, 1, BUFFER_SIZE)] == NEW_LINE_CHAR ) {
                    return scancode;
                }
            } else if(c == TABULATOR_KEY){
                c = TABULATOR_CHAR;
            }

            addCharToBuffer(c, keyboard_options & SHOW_BUFFER_WHILE_TYPING);
        } else if (c == BACKSPACE_KEY && to_write != to_read) {
            DEC_MOD(to_write, BUFFER_SIZE);
            clearPreviousCharacter();
        }
    }

    // Call the registered function for the key, if any
    if (KeyFnMap[scancode].fn != 0) {
        KeyFnMap[scancode].fn(scancode);
    }

    return scancode;

}
