#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <lib.h>

enum KEYS {
    ESCAPE_KEY        = 0x01,
    KEY_1             = 0x02,
    KEY_2             = 0x03,
    KEY_3             = 0x04,
    KEY_4             = 0x05,
    KEY_5             = 0x06,
    KEY_6             = 0x07,
    KEY_7             = 0x08,
    KEY_8             = 0x09,
    KEY_9             = 0x0A,
    KEY_0             = 0x0B,
    MINUS_KEY         = 0x0C,
    EQUALS_KEY        = 0x0D,
    BACKSPACE_KEY     = 0x0E,
    TABULATOR_KEY     = 0x0F,
    Q_KEY             = 0x10,
    W_KEY             = 0x11,
    E_KEY             = 0x12,
    R_KEY             = 0x13,
    T_KEY             = 0x14,
    Y_KEY             = 0x15,
    U_KEY             = 0x16,
    I_KEY             = 0x17,
    O_KEY             = 0x18,
    P_KEY             = 0x19,
    LEFT_BRACKET_KEY  = 0x1A,
    RIGHT_BRACKET_KEY = 0x1B,
    RETURN_KEY        = 0x1C,
    CONTROL_KEY_L     = 0x1D,
    A_KEY             = 0x1E,
    S_KEY             = 0x1F,
    D_KEY             = 0x20,
    F_KEY             = 0x21,
    G_KEY             = 0x22,
    H_KEY             = 0x23,
    J_KEY             = 0x24,
    K_KEY             = 0x25,
    L_KEY             = 0x26,
    SEMICOLON_KEY     = 0x27,
    APOSTROPHE_KEY    = 0x28,
    GRAVE_KEY         = 0x29,
    SHIFT_KEY_L       = 0x2A,
    BACKSLASH_KEY     = 0x2B,
    Z_KEY             = 0x2C,
    X_KEY             = 0x2D,
    C_KEY             = 0x2E,
    V_KEY             = 0x2F,
    B_KEY             = 0x30,
    N_KEY             = 0x31,
    M_KEY             = 0x32,
    COMMA_KEY         = 0x33,
    PERIOD_KEY        = 0x34,
    SLASH_KEY         = 0x35,
    SHIFT_KEY_R       = 0x36,
    KP_ASTERISK_KEY   = 0x37,
    ALT_KEY_L         = 0x38,
    SPACE_KEY         = 0x39,
    CAPS_LOCK_KEY     = 0x3A,
    F1_KEY            = 0x3B,
    F2_KEY            = 0x3C,
    F3_KEY            = 0x3D,
    F4_KEY            = 0x3E,
    F5_KEY            = 0x3F,
    F6_KEY            = 0x40,
    F7_KEY            = 0x41,
    F8_KEY            = 0x42,
    F9_KEY            = 0x43,
    F10_KEY           = 0x44,
    NUM_LOCK_KEY      = 0x45,
    SCROLL_LOCK_KEY   = 0x46,
    KP_HOME_KEY       = 0x47,
    KP_UP_KEY         = 0x48,
    KP_PAGE_UP_KEY    = 0x49,
    KP_MINUS_KEY      = 0x4A,
    KP_LEFT_KEY       = 0x4B,
    KP_BEGIN_KEY      = 0x4C,
    KP_RIGHT_KEY      = 0x4D,
    KP_PLUS_KEY       = 0x4E,
    KP_END_KEY        = 0x4F,
    KP_DOWN_KEY       = 0x50,
    KP_PAGE_DOWN_KEY  = 0x51,
    KP_INSERT_KEY     = 0x52,
    KP_DELETE_KEY     = 0x53,
    F11_KEY           = 0x57,
    F12_KEY           = 0x58
};

#define META_L_KEY (ALT_KEY_L | 0x80)

typedef void (*SpecialKeyHandler)(enum KEYS scancode);

enum KEYBOARD_OPTIONS {
    SHOW_BUFFER_WHILE_TYPING = 0b00000001,
    AWAIT_RETURN_KEY = 0b00000010,
    MODIFY_BUFFER = 0b00000100
};

int8_t getKeyboardCharacter(enum KEYBOARD_OPTIONS keyboard_options);
void addCharToBuffer(int8_t ascii, uint8_t showOutput);
uint16_t clearBuffer();
uint8_t keyboardHandler();

// All special keys *EXCEPT* for TAB and RETURN can be registered
// Printable keys, including tab (`\t`) and return (`\n`) can be obtained via `getKeyboardCharacter` (`getchar`/`sys_read`)
uint8_t registerSpecialKey(enum KEYS scancode, SpecialKeyHandler fn, uint8_t registeredFromKernel);
void clearKeyFnMapNonKernel(SpecialKeyHandler * map);
void restoreKeyFnMapNonKernel(SpecialKeyHandler * map);

#endif