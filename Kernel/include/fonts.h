#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>
#include <video.h>

#define DEFAULT_TEXT_COLOR 0x00FFFFFF
#define DEFAULT_ERROR_COLOR 0x00DE382B
#define DEFAULT_BACKGROUND_COLOR 0x00000000

#define DEFAULT_GLYPH_SIZE_X 8
#define DEFAULT_GLYPH_SIZE_Y 8

#define NEW_LINE_CHAR '\n'
#define TABULATOR_CHAR '\t'
#define CARRIAGE_RETURN_CHAR '\r'
#define ESCAPE_CHAR '\e'
#define TAB_SIZE 4

void putChar(char ascii);
void print(const char * string);
int32_t printToFd(int32_t fd, const char * string, int32_t count);
void newLine();
void printDec(uint64_t value);
void printHex(uint64_t value);
void printBin(uint64_t value);
void clear(void);

void showCursor(void);
void hideCursor(void);
void retractPosition();
void clearPreviousCharacter(void);
uint16_t getXBufferPosition(void);

uint8_t increaseFontSize(void);
uint8_t decreaseFontSize(void);
uint8_t setFontSize(int8_t size);
uint8_t getFontSize(void);
void setTextColor(uint32_t color);
void setBackgroundColor(uint32_t color);
uint32_t getTextColor(void);
uint32_t getBackgroundColor(void);

#endif