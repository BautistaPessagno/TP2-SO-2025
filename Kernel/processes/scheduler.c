// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <defs.h>
#include <lib.h>
#include <linkedListADT.h>
#include <memory_manager.h>
#include <scheduler.h>
#include <stdlib.h>
#include <video.h>
#define QTY_READY_LEVELS 5
#define MAX_PRIORITY 4
#define MIN_PRIORITY 0
#define BLOCKED_INDEX QTY_READY_LEVELS
#define MAX_PROCESSES (1 << 12)
#define IDLE_PID 1
#define QUANTUM_COEF 2

typedef struct SchedulerCDT {
	Node *processes[MAX_PROCESSES];
	LinkedListADT levels[QTY_READY_LEVELS + 1];
	uint16_t currentPid;
	uint16_t nextUnusedPid;
	uint16_t qtyProcesses;
	int8_t remainingQuantum;
	int8_t killFgProcess;
} SchedulerCDT;

SchedulerADT createScheduler() {
	SchedulerADT scheduler = (SchedulerADT) SCHEDULER_ADDRESS;
	for (int i = 0; i < MAX_PROCESSES; i++)
		scheduler->processes[i] = NULL;
	for (int i = 0; i < QTY_READY_LEVELS + 1; i++)
		scheduler->levels[i] = createLinkedListADT();
	scheduler->nextUnusedPid = 0;
	scheduler->killFgProcess = 0;
	return scheduler;
}

SchedulerADT getSchedulerADT() {
	return (SchedulerADT) SCHEDULER_ADDRESS;
}

static uint16_t getNextPid(SchedulerADT scheduler) {
	Process *process = NULL;
	for (int lvl = QTY_READY_LEVELS - 1; lvl >= 0 && process == NULL; lvl--)
		if (!isEmpty(scheduler->levels[lvl]))
			process = (Process *) (getFirst(scheduler->levels[lvl]))->data;

	if (process == NULL)
		return IDLE_PID;
	return process->pid;
}

int32_t setPriority(uint16_t pid, uint8_t newPriority) {
	SchedulerADT scheduler = getSchedulerADT();
	Node *node = scheduler->processes[pid];
	if (node == NULL || pid == IDLE_PID)
		return -1;
	Process *process = (Process *) node->data;
	if (newPriority >= QTY_READY_LEVELS)
		return -1;
	if (process->state == READY || process->state == RUNNING) {
		removeNode(scheduler->levels[process->priority], node);
		scheduler->processes[process->pid] = appendNode(scheduler->levels[newPriority], node);
	}
	process->priority = newPriority;
	return newPriority;
}

int8_t setStatus(uint16_t pid, uint8_t newStatus) {
	SchedulerADT scheduler = getSchedulerADT();
	Node *node = scheduler->processes[pid];
	if (node == NULL || pid == IDLE_PID)
		return -1;
	Process *process = (Process *) node->data;
	ProcessState oldStatus = process->state;
	if (newStatus == RUNNING || newStatus == ZOMBIE || oldStatus == ZOMBIE)
		return -1;
	if (newStatus == process->state)
		return newStatus;

	process->state = newStatus;
	if (newStatus == BLOCKED) {
		removeNode(scheduler->levels[process->priority], node);
		appendNode(scheduler->levels[BLOCKED_INDEX], node);
		// setPriority(pid, process->priority == MAX_PRIORITY ? MAX_PRIORITY : process->priority + 1);
	}
	else if (oldStatus == BLOCKED) {
		removeNode(scheduler->levels[BLOCKED_INDEX], node);
		// Se asume que ya tiene un nivel predefinido
		// appendNode(scheduler->levels[process->priority], node);
		process->priority = MAX_PRIORITY;
		prependNode(scheduler->levels[process->priority], node);
		scheduler->remainingQuantum = 0;
	}
	return newStatus;
}

ProcessState getProcessStatus(uint16_t pid) {
	SchedulerADT scheduler = getSchedulerADT();
	Node *processNode = scheduler->processes[pid];
	if (processNode == NULL)
		return DEAD;
	return ((Process *) processNode->data)->state;
}

void *schedule(void *prevStackPointer) {
	static int firstTime = 1;
	SchedulerADT scheduler = getSchedulerADT();

	scheduler->remainingQuantum--;
	if (!scheduler->qtyProcesses || scheduler->remainingQuantum > 0)
		return prevStackPointer;

	Process *currentProcess;
	Node *currentProcessNode = scheduler->processes[scheduler->currentPid];

	if (currentProcessNode != NULL) {
		currentProcess = (Process *) currentProcessNode->data;
		if (!firstTime)
			currentProcess->stackPos = prevStackPointer;
		else
			firstTime = 0;
		if (currentProcess->state == RUNNING)
			currentProcess->state = READY;

		uint8_t newPriority = currentProcess->priority > 0 ? currentProcess->priority - 1 : currentProcess->priority;
		setPriority(currentProcess->pid, newPriority);
	}

	scheduler->currentPid = getNextPid(scheduler);
	currentProcess = scheduler->processes[scheduler->currentPid]->data;

	
	if (scheduler->killFgProcess && currentProcess->fileDescriptors[STDIN] == STDIN) {
		print("Killing foreground process\n");
		scheduler->killFgProcess = 0;
		if (killCurrentProcess(-1) != -1)
			forceTimerTick();
	}
	scheduler->remainingQuantum = (MAX_PRIORITY - currentProcess->priority);
	currentProcess->state = RUNNING;
	return currentProcess->stackPos;
}

// New API expected by kernel/process subsystem
void sched_init(uint8_t maxPriority) {
	(void)maxPriority; // current implementation uses compile-time QTY_READY_LEVELS/MAX_PRIORITY
	(void)createScheduler();
	SchedulerADT scheduler = getSchedulerADT();
	scheduler->currentPid = 0;
	scheduler->qtyProcesses = 0;
	scheduler->remainingQuantum = 1;
	scheduler->nextUnusedPid = 1;
}

int sched_register_process(Process *process) {
	SchedulerADT scheduler = getSchedulerADT();
	if (process == NULL)
		return -1;
	if (process->pid >= MAX_PROCESSES)
		return -1;
	if (scheduler->processes[process->pid] != NULL)
		return -1;
	Node *processNode = appendElement(scheduler->levels[process->priority], (void *) process);
	if (processNode == NULL)
		return -1;
	scheduler->processes[process->pid] = processNode;
	if (process->pid == IDLE_PID) {
		removeNode(scheduler->levels[process->priority], processNode);
	}
	scheduler->qtyProcesses++;
	return 0;
}

uint16_t sched_getpid() {
	return getpid();
}

void sched_yield() {
	yield();
}

int32_t sched_kill_process(uint16_t pid, int32_t retValue) {
	return killProcess(pid, retValue);
}

int32_t sched_set_priority(uint16_t pid, uint8_t newPriority) {
	return setPriority(pid, newPriority);
}

int8_t sched_set_status(uint16_t pid, uint8_t newStatus) {
	return setStatus(pid, newStatus);
}

// Wrapper called from _irq00Handler in interrupts.asm
void *sched_tick_isr(void *prevStackPointer) {
	return schedule(prevStackPointer);
}

static void destroyZombie(SchedulerADT scheduler, Process *zombie) {
	Node *zombieNode = scheduler->processes[zombie->pid];
	scheduler->qtyProcesses--;
	scheduler->processes[zombie->pid] = NULL;
	freeProcess(zombie);
	mm_free(zombieNode);
}

int32_t killCurrentProcess(int32_t retValue) {
	SchedulerADT scheduler = getSchedulerADT();
	return killProcess(scheduler->currentPid, retValue);
}

int32_t killProcess(uint16_t pid, int32_t retValue) {
	SchedulerADT scheduler = getSchedulerADT();
	Node *processToKillNode = scheduler->processes[pid];
	if (processToKillNode == NULL)
		return -1;
	Process *processToKill = (Process *) processToKillNode->data;
	if (processToKill->state == ZOMBIE || processToKill->unkillable)
		return -1;

	closeFileDescriptors(processToKill);

	uint8_t priorityIndex = processToKill->state != BLOCKED ? processToKill->priority : BLOCKED_INDEX;
	removeNode(scheduler->levels[priorityIndex], processToKillNode);
	processToKill->retValue = retValue;

	processToKill->state = ZOMBIE;

	begin(processToKill->zombieChildren);
	while (hasNext(processToKill->zombieChildren)) {
		destroyZombie(scheduler, (Process *) next(processToKill->zombieChildren));
	}

	Node *parentNode = scheduler->processes[processToKill->parentPid];
	if (parentNode != NULL && ((Process *) parentNode->data)->state != ZOMBIE) {
		Process *parent = (Process *) parentNode->data;
		appendNode(parent->zombieChildren, processToKillNode);
		if (processIsWaiting(parent, processToKill->pid))
			setStatus(processToKill->parentPid, READY);
	}
	else {
		destroyZombie(scheduler, processToKill);
	}
	if (pid == scheduler->currentPid)
		yield();
	return 0;
}

uint16_t getpid() {
	SchedulerADT scheduler = getSchedulerADT();
	return scheduler->currentPid;
}

ProcessSnapshotList *getProcessSnapshot() {
	SchedulerADT scheduler = getSchedulerADT();
	ProcessSnapshotList *snapshotsArray = mm_malloc(sizeof(ProcessSnapshotList));
	if (snapshotsArray == NULL)
		return NULL;
	if (scheduler->qtyProcesses == 0) {
		mm_free(snapshotsArray);
		return NULL;
	}
	ProcessSnapshot *psArray = mm_malloc(scheduler->qtyProcesses * sizeof(ProcessSnapshot));
	if (psArray == NULL) {
		mm_free(snapshotsArray);
		return NULL;
	}
	int processIndex = 0;

	Node *idleNode = scheduler->processes[IDLE_PID];
	if (idleNode != NULL) {
		loadSnapshot(&psArray[processIndex++], (Process *) idleNode->data);
	}
	for (int lvl = QTY_READY_LEVELS; lvl >= 0; lvl--) { // Se cuentan tambien los bloqueados
		begin(scheduler->levels[lvl]);
		while (hasNext(scheduler->levels[lvl])) {
			Process *nextProcess = (Process *) next(scheduler->levels[lvl]);
			loadSnapshot(&psArray[processIndex], nextProcess);
			processIndex++;
			if (nextProcess->state != ZOMBIE) {
				getZombiesSnapshots(processIndex, psArray, nextProcess);
				processIndex += getLength(nextProcess->zombieChildren);
			}
		}
	}
	snapshotsArray->length = scheduler->qtyProcesses;
	snapshotsArray->snapshotList = psArray;
	return snapshotsArray;
}

int32_t waitpid(uint16_t pid) {
	SchedulerADT scheduler = getSchedulerADT();
	Node *zombieNode = scheduler->processes[pid];
	if (zombieNode == NULL)
		return -1;
	Process *zombieProcess = (Process *) zombieNode->data;
	if (zombieProcess->parentPid != scheduler->currentPid)
		return -1;

	Process *parent = (Process *) scheduler->processes[scheduler->currentPid]->data;
	parent->waitingForPid = pid;
	if (zombieProcess->state != ZOMBIE) {
		setStatus(parent->pid, BLOCKED);
		yield();
	}
	removeNode(parent->zombieChildren, zombieNode);
	destroyZombie(scheduler, zombieProcess);
	return zombieProcess->retValue;
}

int32_t processIsAlive(uint16_t pid) {
	SchedulerADT scheduler = getSchedulerADT();
	Node *processNode = scheduler->processes[pid];
	return processNode != NULL && ((Process *) processNode->data)->state != ZOMBIE;
}

void yield() {
	SchedulerADT scheduler = getSchedulerADT();
	scheduler->remainingQuantum = 0;
	forceTimerTick();
}

int8_t changeFD(uint16_t pid, uint8_t position, int16_t newFd) {
	SchedulerADT scheduler = getSchedulerADT();
	Node *processNode = scheduler->processes[pid];
	if (pid == IDLE_PID || processNode == NULL)
		return -1;
	Process *process = (Process *) processNode->data;
	process->fileDescriptors[position] = newFd;
	return 0;
}

int16_t getCurrentProcessFileDescriptor(uint8_t fdIndex) {
	SchedulerADT scheduler = getSchedulerADT();
	Process *process = scheduler->processes[scheduler->currentPid]->data;
	return process->fileDescriptors[fdIndex];
}

void killForegroundProcess() {
	SchedulerADT scheduler = getSchedulerADT();
	scheduler->killFgProcess = 1;
}
