#include <ansiColors.h>
#include <syscalls.h>

static void setANSIProp(uint8_t prop);
void parseANSI(const char * string, int * i);

static void setANSIProp(uint8_t prop) {
    int32_t (*set_fn)(uint32_t) = sys_fonts_text_color;

    if ((prop >= 40 && prop <= 47) || (prop >= 100 && prop <= 107)) {
        prop -= 10;
        set_fn = sys_fonts_background_color;
    }

    switch (prop) {
        case 0:
            sys_fonts_text_color(0x00FFFFFF);
            sys_fonts_background_color(0x00000000);
            break;
        case 30:
            set_fn(0x00000000);
            break;
        case 31:
            set_fn(0x00DE382B);
            break;
        case 32:
            set_fn(0x0039B54A);
            break;
        case 33:
            set_fn(0x00FFC706);
            break;
        case 34:
            set_fn(0x00006FB8);
            break;
        case 35:
            set_fn(0x00762671);
            break;
        case 36:
            set_fn(0x002CB5E9);
            break;
        case 37:
            set_fn(0x00CCCCCC);
            break;
        case 90:
            set_fn(0x00808080);
        case 91:
            set_fn(0x00FF0000);
            break;
        case 92:
            set_fn(0x0000FF00);
            break;
        case 93:
            set_fn(0x00FFFF00);
            break;
        case 94:
            set_fn(0x000000FF);
            break;
        case 95:    
            set_fn(0x00FF00FF);
            break;
        case 96:
            set_fn(0x0000FFFF);
            break;
        case 97:
            set_fn(0x00FFFFFF);
            break;
        default:
            break;
    }
}

void parseANSI(const char * string, int * i) {
    // \e[0;31mExample\e[0m"
    // \e[0;31m -> Set text_color to red
    // \e[0m -> Reset text_color

    if (string[(*i) + 1] != '[' || string[(*i) + 2] == 0) return;

    (*i) += 2;

    uint8_t prop = 0;

    do {
        if (string[*i] == ';' || string[*i] == 'm') {
            setANSIProp(prop);
            prop = 0;
        } else {
            prop = prop * 10 + (string[*i] - '0');
        }
    } while (string[(*i)++] != 'm');
}