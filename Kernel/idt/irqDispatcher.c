#include <time.h>
#include <stdint.h>
#include <keyboard.h>

static uint8_t int_20();
static uint8_t int_21();

static uint8_t (*interruptions[]) (void) = {
	int_20,
	int_21
};

uint8_t irqDispatcher(uint64_t irq) {
	if (irq < 2) {
		return interruptions[irq]();
	}
	return 0;
}

static uint8_t int_20() {
	timer_handler();
	return 0;
}

static uint8_t int_21() {
	return keyboardHandler();
}
