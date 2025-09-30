#ifndef _SYSCALL_DISPATCHER_H_
#define _SYSCALL_DISPATCHER_H_

#include <stdint.h>
#include <keyboard.h>

typedef struct {
    int64_t r15;
	int64_t r14;
	int64_t r13;
	int64_t r12;
	int64_t r11;
	int64_t r10;
	int64_t r9;
	int64_t r8;
	int64_t rsi;
	int64_t rdi;
	int64_t rbp;
	int64_t rdx;
	int64_t rcx;
	int64_t rbx;
	int64_t rax;
	int64_t rip;
} Registers;

int32_t syscallDispatcher(Registers * registers);

// Linux syscall prototypes
int32_t sys_write(int32_t fd, char * __user_buf, int32_t count);
int32_t sys_read(int32_t fd, signed char * __user_buf, int32_t count);

// Custom syscall prototypes
int32_t sys_start_beep(uint32_t nFrequence);
int32_t sys_stop_beep(void);
int32_t sys_fonts_text_color(uint32_t color);
int32_t sys_fonts_background_color(uint32_t color);
int32_t sys_fonts_decrease_size(void);
int32_t sys_fonts_increase_size(void);
int32_t sys_fonts_set_size(uint8_t size);
int32_t sys_clear_screen(void);
int32_t sys_clear_input_buffer(void);
uint16_t sys_window_width(void);
uint16_t sys_window_height(void);

// Date syscall prototypes
int32_t sys_hour(int * hour);
int32_t sys_minute(int * minute);
int32_t sys_second(int * second);


int32_t sys_circle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter);
// Draw rectangle syscall prototype
int32_t sys_rectangle(uint32_t color, uint64_t width_pixels, uint64_t height_pixels, uint64_t initial_pos_x, uint64_t initial_pos_y);
int32_t sys_fill_video_memory(uint32_t hexColor);

// Custom exec syscall prototype
int32_t sys_exec(int32_t (*fnPtr)(void));

// Custom keyboard syscall prototypes
int32_t sys_register_key(uint8_t scancode, SpecialKeyHandler fn);

// System sleep
int32_t sys_sleep_milis(uint32_t milis);

// Register snapshot
int32_t sys_get_register_snapshot(int64_t * registers);

// Get character without showing
int32_t sys_get_character_without_display(void);

#endif