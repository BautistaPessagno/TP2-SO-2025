#ifndef VIDEO_DRIVER_H
#define VIDEO_DRIVER_H

#include <stdint.h>

void putPixel(uint32_t hexColor, uint64_t x, uint64_t y);
void drawCircle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter);
void drawRectangle(uint32_t hexColor, uint64_t width, uint64_t height, uint64_t initial_pos_x, uint64_t initial_pos_y);
void fillVideoMemory(uint32_t hexColor);

uint16_t getWindowWidth(void);
uint16_t getWindowHeight(void);

void scrollVideoMemoryUp(uint16_t scroll, uint32_t fillColor);

#endif