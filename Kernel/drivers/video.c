/**
 * Note: Certain functions within this file are HOT PATHS.
 * They have been optimized as best as possible.
 * 
 * This file is excluded from the main compilation rules, and is always compiled with -O3 (regardless of the main compilation rules).
 */

#include <video.h>
#include <interrupts.h>

struct vbe_mode_info_structure {
	uint16_t attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	uint8_t window_a;			// deprecated
	uint8_t window_b;			// deprecated
	uint16_t granularity;		// deprecated; used while calculating bank numbers
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	uint16_t pitch;			// number of bytes per horizontal line
	uint16_t width;			// width in pixels
	uint16_t height;			// height in pixels
	uint8_t w_char;			// unused...
	uint8_t y_char;			// ...
	uint8_t planes;
	uint8_t bpp;			// bits per pixel in this mode
	uint8_t banks;			// deprecated; total number of banks in this mode
	uint8_t memory_model;
	uint8_t bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	uint8_t image_pages;
	uint8_t reserved0;
 
	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;
 
	uint32_t framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	uint8_t reserved1[206];
} __attribute__ ((packed));

typedef struct vbe_mode_info_structure * VBEInfoPtr;

VBEInfoPtr VBE_mode_info = (VBEInfoPtr) 0x0000000000005C00;

void putPixel(uint32_t hexColor, uint64_t x, uint64_t y) {
    uint8_t * framebuffer = (uint8_t * )(unsigned long long)(VBE_mode_info->framebuffer);
    uint64_t offset = (x * ((VBE_mode_info->bpp) >> 3)) + (y * VBE_mode_info->pitch);
	uint8_t b = (hexColor) & 0xFF, g = (hexColor >> 8) & 0xFF, r = (hexColor >> 16) & 0xFF;

    framebuffer[offset]     =  b;
    framebuffer[offset+1]   =  g;
    framebuffer[offset+2]   =  r;
}

void drawRectangle(uint32_t hexColor, uint64_t width, uint64_t height, uint64_t initial_pos_x, uint64_t initial_pos_y){
	for(uint64_t y = initial_pos_y; y - initial_pos_y < height; y++){
		for(uint64_t x = initial_pos_x; x - initial_pos_x < width; x++){
			putPixel(hexColor, x, y);
		}
	}
}

void drawCircle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter) {
    int64_t radius = diameter / 2;
    int64_t centerX = topLeftX + radius;
    int64_t centerY = topLeftY + radius;
    
    for (int64_t y = -radius; y < radius; y++) {
        for (int64_t x = -radius; x < radius; x++) {
            if (x * x + y * y <= radius * radius) {
                putPixel(hexColor, centerX + x, centerY + y);
            }
        }
    }
}


void fillVideoMemory(uint32_t hexColor) {
	uint8_t * framebuffer = (uint8_t * )(unsigned long long)(VBE_mode_info->framebuffer);
	uint16_t width = getWindowWidth();
	uint16_t height = getWindowHeight();

	uint8_t b = (hexColor) & 0xFF, g = (hexColor >> 8) & 0xFF, r = (hexColor >> 16) & 0xFF;
	uint64_t yoffset;
	for (uint16_t y = 0; y < height; y++) {
		yoffset = (y * VBE_mode_info->pitch);
		for (uint16_t x = 0; x < width; x++) {
			uint64_t offset = (x * ((VBE_mode_info->bpp) >> 3)) + yoffset;
			framebuffer[offset] = b;
			framebuffer[offset + 1] = g;
			framebuffer[offset + 2] = r;
		}
	}
}

uint16_t getWindowHeight() {
	return VBE_mode_info->height;
}

uint16_t getWindowWidth() {
	return VBE_mode_info->width;
}

void scrollVideoMemoryUp(uint16_t scroll, uint32_t fillColor) {
	_cli();

	uint8_t * framebuffer = (uint8_t * )(unsigned long long)(VBE_mode_info->framebuffer);
	uint16_t width = getWindowWidth();
	uint16_t height = getWindowHeight();
	
	uint8_t b = (fillColor) & 0xFF, g = (fillColor >> 8) & 0xFF, r = (fillColor >> 16) & 0xFF;

	// Iterating over Y, then X
	// -> Memory is contiguous in the framebuffer, increased cached hits, reduced tearing
	uint64_t yoffset, ynoffset, offset, new_offset, xo;
	for (uint16_t y = 0; y < height - scroll; y++) {
		yoffset = (y * VBE_mode_info->pitch);
		ynoffset = ((y + scroll) * VBE_mode_info->pitch);
		for (uint16_t x = 0; x < width; x++) {
			xo = (x * ((VBE_mode_info->bpp) >> 3));
			offset = xo + yoffset;
			new_offset = xo + ynoffset;
			framebuffer[offset] = framebuffer[new_offset];
			framebuffer[offset + 1] = framebuffer[new_offset + 1];
			framebuffer[offset + 2] = framebuffer[new_offset + 2];
		}
	}

	for (uint16_t y = height - scroll; y < height; y++) {
		yoffset = (y * VBE_mode_info->pitch);
		for (uint16_t x = 0; x < width; x++) {
			offset = (x * ((VBE_mode_info->bpp) >> 3)) + yoffset;
			framebuffer[offset] = b;
			framebuffer[offset + 1] = g;
			framebuffer[offset + 2] = r;
		}
	}

	_sti();
}
