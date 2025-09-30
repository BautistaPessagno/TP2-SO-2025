/**
 * Note: Certain functions within this file are HOT PATHS.
 * They have been optimized as best as possible.
 * 
 * This file is excluded from the main compilation rules, and is always compiled with -O3 (regardless of the main compilation rules).
 */

#include <fonts.h>
#include <keyboard.h>
#include <video.h>

/* 
    Note: An attempt was made to use the Linux kernel's Solarize.12x29.psf (https://wiki.osdev.org/PC_Screen_Font). Now only the pain remains.
    The Makefile recipe is left here in case an(other) insane attempt at porting it is made. 

    For the time being a simpler bitmapped font embedded into a header file is used.
    Note: Symbol table entries on Solarize are: 
    - _binary___font_assets_Solarize_12x29_psf_start
    - _binary___font_assets_Solarize_12x29_psf_end
    - _binary___font_assets_Solarize_12x29_psf_size

    NOT the same as the ones listed on https://wiki.osdev.org/PC_Screen_Font

    font.o:
        objcopy -O elf64-x86-64 -B i386 -I binary ./font_assets/Solarize.12x29.psf font.o
 */

#include "include/font_basic_8x8.h"

#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_STDERR 2

#define MAX(a,b) ((a) > (b) ? (a) : (b))

static uint16_t glyphSizeX = DEFAULT_GLYPH_SIZE_X;
static uint16_t glyphSizeY = DEFAULT_GLYPH_SIZE_Y;
static uint16_t fontSize = 1;

static char * bitmap = (char *) font8x8_basic;

static int32_t xBufferPosition;
static int32_t yBufferPosition;

static uint16_t maxGlyphSizeYOnLine = DEFAULT_GLYPH_SIZE_Y;

static uint8_t dirty_line;

static uint32_t text_color = DEFAULT_TEXT_COLOR;
static uint32_t background_color = DEFAULT_BACKGROUND_COLOR;
static uint8_t file_descriptor = FD_STDOUT;

void setTextColor(uint32_t color) {
    text_color = color;
}

void setBackgroundColor(uint32_t color) {
    background_color = color;
}

uint32_t getTextColor(void) {
    return text_color;
}

uint32_t getBackgroundColor(void) {
    return background_color;
}

uint16_t getXBufferPosition(void) {
    return xBufferPosition;
}

static char buffer[64] = { '0' };

static inline void renderFromBitmap(char * bitmap, uint64_t xBase, uint64_t yBase);
static inline void renderAscii(char ascii, uint64_t x, uint64_t y);

void showCursor(void);
void hideCursor(void);
static void scrollBufferPositionIfNeeded(void);
void clearPreviousCharacter(void);

static uint32_t uintToBase(uint64_t value, char * buffer, uint32_t base);
static void printBase(uint64_t value, uint32_t base);
static inline int64_t strlen(const char * str);

// * Uses inline to avoid stack frames on hot paths *
static inline void renderFromBitmap(char * bitmap, uint64_t xBase, uint64_t yBase) {
    int xs, xo;
    for (int x = 0; x < glyphSizeX * fontSize; x++) {
        xs = xBase + x;
        xo = x / fontSize;
        for (int y = 0; y < glyphSizeY * fontSize; y++) {
            // Read into char * slice and mask
            putPixel(*(bitmap + (y / fontSize)) & (1 << xo) ? text_color : background_color, xs, yBase + y);
        }
    }
}

// * Uses inline to avoid stack frames on hot paths *
// `x` and `y` are the TOP LEFT corner positions
static inline void renderAscii(char ascii, uint64_t x, uint64_t y) {
    if (ascii < 128) {
        // The function only takes in a slice of the whole matrix
        renderFromBitmap(bitmap + (ascii * glyphSizeY), x, y);
    }
}

static void scrollBufferPositionIfNeeded(void) {
    if (yBufferPosition + glyphSizeY * fontSize > getWindowHeight()) {
        scrollVideoMemoryUp(glyphSizeY * fontSize, DEFAULT_BACKGROUND_COLOR);
        yBufferPosition -= glyphSizeY * fontSize;
    }
}

// `ascii` ASCII character to print (0-127)
void putChar(char ascii) {
    dirty_line = 1;
    switch (ascii){
        case NEW_LINE_CHAR:
            hideCursor();
            newLine();
            break;
        case CARRIAGE_RETURN_CHAR:
            hideCursor();
            while (xBufferPosition > 0) {
                retractPosition();
            }
            break;
        case TABULATOR_CHAR:
            do {
                putChar(' ');
            } while(xBufferPosition % (TAB_SIZE * glyphSizeX * fontSize) != 0);
            break;
        default:
            if (xBufferPosition + glyphSizeX * fontSize > getWindowWidth()) {
                newLine();
            }

            renderAscii(ascii, xBufferPosition, yBufferPosition);
            xBufferPosition += glyphSizeX * fontSize;
            break;
    }
}

int32_t printToFd(int32_t fd, const char * string, int32_t count) {
    if (fd != file_descriptor) {
        switch (fd) {
            case FD_STDIN:
                int i = 0;
                for ( ; i < count; i++ ) {
                    addCharToBuffer(string[i], 1);
                }
                return i;
            case FD_STDOUT:
                text_color = DEFAULT_TEXT_COLOR;
                background_color = DEFAULT_BACKGROUND_COLOR;
                file_descriptor = fd;
                break;
            case FD_STDERR:
                text_color = DEFAULT_ERROR_COLOR;
                background_color = DEFAULT_BACKGROUND_COLOR;
                file_descriptor = fd;
                break;
            default:
        }
    }

    int i = 0;
    for ( ; i < count; i++ ) {
        putChar(string[i]);
    }

    return i;
}

// Prints `string` Null terminated string to `STDOUT`
void print(const char * string) {
    printToFd(FD_STDOUT, string, strlen(string));
}

// Jumps to the next line, does not print an empty line
void newLine(void) {
    dirty_line = 0;
    yBufferPosition += maxGlyphSizeYOnLine;
    xBufferPosition = 0;
    maxGlyphSizeYOnLine = fontSize * glyphSizeY;
    scrollBufferPositionIfNeeded();
}

void printDec(uint64_t value) {
    printBase(value, 10);
}

void printHex(uint64_t value) {
    printBase(value, 16);
}

void printBin(uint64_t value) {
    printBase(value, 2);
}

void clear(void) {
    fillVideoMemory(DEFAULT_BACKGROUND_COLOR);
    xBufferPosition = 0;
    yBufferPosition = 0;
}

void retractPosition() {
    uint16_t window_width = getWindowWidth();
    
    if(xBufferPosition == 0){
        yBufferPosition -= glyphSizeY * fontSize;
        if (yBufferPosition < 0) {
            yBufferPosition = 0;
            return;
        }
        xBufferPosition = window_width - (window_width % (fontSize * glyphSizeX));
    }

    xBufferPosition -= glyphSizeX * fontSize;
}

void clearPreviousCharacter(void){
    hideCursor();
    retractPosition();
    showCursor();
}

void showCursor(void) {
    putChar('|');
    retractPosition();
}

void hideCursor(void) {
    putChar(' ');
    retractPosition();
}

uint8_t increaseFontSize(void) {
    fontSize = fontSize > 9 ? fontSize : fontSize + 1;
    maxGlyphSizeYOnLine =  dirty_line == 1 ? MAX(maxGlyphSizeYOnLine, glyphSizeY * fontSize) : (glyphSizeY * fontSize);
    scrollBufferPositionIfNeeded();
    return fontSize;
}

uint8_t decreaseFontSize(void) {
    fontSize = fontSize <= 1 ? fontSize : fontSize - 1;
    maxGlyphSizeYOnLine = dirty_line == 1 ? MAX(maxGlyphSizeYOnLine, glyphSizeY * fontSize) : (glyphSizeY * fontSize);
    return fontSize;
}

uint8_t setFontSize(int8_t size) {
    fontSize = (size < 1 ? 1 : size > 10 ? 10 : size);
    maxGlyphSizeYOnLine = dirty_line == 1 ? MAX(maxGlyphSizeYOnLine, glyphSizeY * fontSize) : (glyphSizeY * fontSize);
    return fontSize;
}

uint8_t getFontSize(void) {
    return fontSize;
}

static void printBase(uint64_t value, uint32_t base) {
    uintToBase(value, buffer, base);
    print(buffer);
}

static uint32_t uintToBase(uint64_t value, char * buffer, uint32_t base) {
	char *p = buffer;
	char *p1, *p2;
	uint32_t digits = 0;

	// Calculate characters for each digit
	do
	{
		uint32_t remainder = value % base;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
		digits++;
	}
	while (value /= base);

	// Terminate string in buffer.
    // @todo Could this overflow?
	*p = 0;

	// Reverse string in buffer.
	p1 = buffer;
	p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}

	return digits;
}

static inline int64_t strlen(const char * str) {
    int64_t length = 0;
    while (str[length] != 0) {
        length++;
    }
    return length;
}
