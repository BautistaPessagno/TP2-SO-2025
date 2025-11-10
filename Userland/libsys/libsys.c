// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <sys.h>
#include <syscalls.h>

void startBeep(uint32_t nFrequence) {
    sys_start_beep(nFrequence);
}

void stopBeep(void) {
    sys_stop_beep();
}

// ============================
// Process and synchronization wrappers
// ============================
extern int32_t sys_getpid(void);
extern int32_t sys_create_process(int (*code)(int, char **), char **args, const char *name, uint8_t priority, const int16_t fileDescriptors[3]);
extern int32_t sys_nice_proc(uint64_t pid, uint64_t newPrio);
extern int32_t sys_kill_proc(uint64_t pid);
extern int32_t sys_block_proc(uint64_t pid);
extern int32_t sys_unblock_proc(uint64_t pid);
extern int32_t sys_sem_init(uint16_t sem_id, uint64_t initialValue);
extern int32_t sys_sem_open(uint16_t sem_id, uint64_t initialValue);
extern int32_t sys_sem_wait(uint16_t sem_id);
extern int32_t sys_sem_post(uint16_t sem_id);
extern int32_t sys_sem_close(uint16_t sem_id);
extern int32_t sys_sem_destroy(uint16_t sem_id);
extern int32_t sys_yield_proc(void);
extern int32_t sys_wait_proc(int64_t pid);
extern int32_t sys_mm_state(void *state);
extern int32_t sys_print_ps(void);

int32_t getpid(void) {
    return sys_getpid();
}

int32_t waitpid(int32_t pid) {
    return sys_wait_proc(pid);
}

int32_t yield(void) {
    return sys_yield_proc();
}

int32_t createProcessWithFds(int (*code)(int, char **), char **args, const char *name, uint8_t priority, const int16_t fileDescriptors[3]) {
    return sys_create_process(code, args, name, priority, fileDescriptors);
}

int32_t killProcess(uint16_t pid) {
    return sys_kill_proc(pid);
}

int32_t nice(uint16_t pid, uint8_t priority) {
    return sys_nice_proc(pid, priority);
}

int32_t block(uint16_t pid) {
    return sys_block_proc(pid);
}

int32_t unblock(uint16_t pid) {
    return sys_unblock_proc(pid);
}

int32_t semInit(uint16_t sem_id, uint32_t initialValue) {
    return sys_sem_init(sem_id, initialValue);
}

int32_t semOpen(uint16_t sem_id, uint32_t initialValue) {
    return sys_sem_open(sem_id, initialValue);
}

int32_t semWait(uint16_t sem_id) {
    return sys_sem_wait(sem_id);
}

int32_t semPost(uint16_t sem_id) {
    return sys_sem_post(sem_id);
}

int32_t semClose(uint16_t sem_id) {
    return sys_sem_close(sem_id);
}

int32_t semDestroy(uint16_t sem_id) {
    return sys_sem_destroy(sem_id);
}

void setTextColor(uint32_t color) {
    sys_fonts_text_color(color);
}

void setBackgroundColor(uint32_t color) {
    sys_fonts_background_color(color);
}

uint8_t increaseFontSize(void) {
    return sys_fonts_increase_size();
}

uint8_t decreaseFontSize(void) {
    return sys_fonts_decrease_size();
}

uint8_t setFontSize(uint8_t size) {
    return sys_fonts_set_size(size);
}

void getDate(int * hour, int * minute, int * second) {
    sys_hour(hour);
    sys_minute(minute);
    sys_second(second);
}

void clearScreen(void) {
    sys_clear_screen();
}

void clearInputBuffer(void) {
    sys_clear_input_buffer();
}

void drawCircle(uint32_t color, long long int topleftX, long long int topLefyY, long long int diameter) {
    sys_circle(color, topleftX, topLefyY, diameter);
}

void drawRectangle(uint32_t color, long long int width_pixels, long long int height_pixels, long long int initial_pos_x, long long int initial_pos_y){
    sys_rectangle(color, width_pixels, height_pixels, initial_pos_x, initial_pos_y);
}

void fillVideoMemory(uint32_t hexColor) {
    sys_fill_video_memory(hexColor);
}

int32_t exec(int32_t (*fnPtr)(void)) {
    return sys_exec(fnPtr);
}

void registerKey(enum REGISTERABLE_KEYS scancode, void (*fn)(enum REGISTERABLE_KEYS scancode)) {
    sys_register_key(scancode, fn);
}


int getWindowWidth(void) {
    return sys_window_width();
}

int getWindowHeight(void) {
    return sys_window_height();
}

void sleep(uint32_t miliseconds) {
    sys_sleep_milis(miliseconds);
}

int32_t getRegisterSnapshot(int64_t * registers) {
    return sys_get_register_snapshot(registers);
}

int32_t getCharacterWithoutDisplay(void) {
    return sys_get_character_without_display();
}

int32_t getMemoryState(MMState *state) {
    return sys_mm_state((void *)state);
}

int32_t printProcesses(void) {
    return sys_print_ps();
}

extern int32_t sys_pipe_get(void);
int16_t pipeGet(void) {
    return (int16_t) sys_pipe_get();
}
void *malloc(uint64_t size) {
    return sys_malloc(size);
}

int64_t free(void *ptr) {
    return sys_free(ptr);
}