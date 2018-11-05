#ifndef ENUMS_H_
#define ENUMS_H_

enum tareas
{
	//Connections
	NEW_DMA_CONNECTION = 1,
	NEW_CPU_CONNECTION = 2,
	NEW_FS_CONNECTION = 3,
	NEW_MEMORY_CONNECTION = 4,
	NEW_SCHEDULER_CONNECTION = 5,

	//Tareas CPU-Planificador
	COMPLETED_INSTRUCTION = 6,	//Used by the scheduler to help keep track of how many instructions were executed at a given time (useful to get metrics)
	BLOCK_PROCESS_INIT = 7,	//This task is used when I need to receive only the process id, not the PCB
	BLOCK_PROCESS = 8,
	PROCESS_ERROR = 9, 		//Sent by the CPU & DMA; due to invalid FS/memory accesses
							//Or not enough memory space to load files/scripts
	QUANTUM_END = 10,
	//Maybe I need to add more

	//Tareas DMA-Planificador
	UNLOCK_PROCESS = 13,	//El DMA paso la data requerida por el proceso a memoria
};

enum exit_codes
{
	ExitCode_CONFIGNOTFOUND = -10
};

#endif /* ENUMS_H_ */
