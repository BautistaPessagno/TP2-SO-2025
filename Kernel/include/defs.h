/***************************************************
  Defs.h
****************************************************/

#ifndef _DEFS_
#define _DEFS_

/* Segment access flags */
#define ACS_PRESENT     0x80            /* segment present in memory */
#define ACS_CSEG        0x18            /* code segment */
#define ACS_DSEG        0x10            /* data segment */
#define ACS_READ        0x02            /* read segment */
#define ACS_WRITE       0x02            /* write segment */
#define ACS_IDT         ACS_DSEG
#define ACS_INT_386 	  0x0E            /* Interrupt GATE 32 bits */
#define ACS_INT         ( ACS_PRESENT | ACS_INT_386 )

#define ACS_CODE        (ACS_PRESENT | ACS_CSEG | ACS_READ)
#define ACS_DATA        (ACS_PRESENT | ACS_DSEG | ACS_WRITE)
#define ACS_STACK       (ACS_PRESENT | ACS_DSEG | ACS_WRITE)

/* Direcciones del Memory Manager */

#define SYSTEM_VARIABLES 0x5A00
#define MEMORY_MANAGER_ADDRESS 0x50000	  // MemoryManagerCDT
#define SCHEDULER_ADDRESS 0x60000		  // SchedulerCDT
#define SEMAPHORE_MANAGER_ADDRESS 0x70000 // SemaphoreCDT
#define PIPE_MANAGER_ADDRESS 0x80000	  // PipeManagerCDT

#endif