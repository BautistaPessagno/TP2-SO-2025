#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <moduleLoader.h>
#include <video.h>
#include <idtLoader.h>
#include <fonts.h>
#include <syscallDispatcher.h>
#include <sound.h>
#include <memory_manager.h>
#include <semaphore_manager.h>
#include <pipe_manager.h>
#include <scheduler.h>
#include <processes.h>
#include <keyboard.h>

// extern uint8_t text;
// extern uint8_t rodata;
// extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const shellModuleAddress = (void *)0x400000;
static void * const snakeModuleAddress = (void *)0x500000;

typedef int (*EntryPoint)();

static uint16_t next_pid = 1;

static void createProcess(MainFunction code,
                          char **args,
                          const char *name,
                          uint8_t priority,
                          const int16_t fileDescriptors[3],
                          uint8_t unkillable) {
    Process *p = (Process*)mm_malloc(sizeof(Process));
    if (p == 0) return;
    uint16_t parent = sched_getpid(); // 0 if none yet
    uint16_t pid = next_pid++;
    initProcess(p, pid, parent, code, args, name, priority, fileDescriptors, unkillable);
    (void)sched_register_process(p);
}

// Idle process: crea la shell y hace hlt para siempre
// preguntar en revision, el problema esta en la shell casi seguro, arranca aca el seguimiento
static int idle(int argc, char **argv) {
    // esto es omitible, creo
    // son expresiones vacias que "consumen" las variables sin hacer nada
    (void)argc; (void)argv;

    char *argsShell[2] = {"shell", NULL};
    int16_t fdsShell[3] = {STDIN, STDOUT, STDERR};
    // le pasa max priority 4 que es mas alta que idle
    createProcess((MainFunction)shellModuleAddress, argsShell, "shell", 4, fdsShell, 1);

    while (1) { _hlt(); }
    return 0;
}

void clearBSS(void * bssAddress, uint64_t bssSize){
	memset(bssAddress, 0, bssSize);
}

void * getStackBase() {
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8				//The size of the stack itself, 32KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

void * initializeKernelBinary(){
    void * moduleAddresses[] = {
        shellModuleAddress,
        snakeModuleAddress,
    };

    loadModules(&endOfKernelBinary, moduleAddresses);

    clearBSS(&bss, &endOfKernel - &bss);

    // Core managers and scheduler
    create_memory_manager(0);
    sched_init(4);
    sched_set_idle_stack(getStackBase());
    createSemaphoreManager();
    createPipeManager();
    // Keyboard driver is interrupt-driven; no explicit init function required

    return getStackBase();
}

int main(){ 
    setFontSize(2);

    // Create idle process (which will spawn shell)
    char *argsIdle[3] = {"idle", "Hm?", NULL};
    int16_t fdIdle[3] = {STDIN, STDOUT, STDERR};
    next_pid = 1; // ensure PID 1 for idle
    createProcess((MainFunction)&idle, argsIdle, "idle", 1, fdIdle, 1);

    load_idt();

    // Halt and let timer IRQ trigger first schedule
    for(;;) { _hlt(); }

    __builtin_unreachable();
    return 0;
}
