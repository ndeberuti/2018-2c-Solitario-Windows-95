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

	//CPU-Scheduler Tasks
	COMPLETED_INSTRUCTION = 6,	//Used by the scheduler to help keep track of how many instructions were executed at a given time (useful to get metrics)
	BLOCK_PROCESS_INIT = 7,	//This task is used when I need to receive only the process id, not the PCB
	BLOCK_PROCESS = 8,
	PROCESS_ERROR = 9, 		//Sent by the CPU & DMA; due to invalid FS/memory accesses
							//Or not enough memory space to load files/scripts; the Process is killed
	QUANTUM_END = 10,
	INITIALIZE_PROCESS = 11,	//The scheduler tells the CPU to initialize a process
	EXECUTE_PROCESS = 12,		//The scheduler tells the CPU to execute a process
	CHECK_IF_FILE_OPEN = 13,	//The CPU asks the scheduler to see if a file is open for a process
	WAIT_RESOURCE = 14,
	SIGNAL_RESOURCE = 15,
	WAIT_ERROR = 16,		//The scheduler tells the CPU it could not reserve that resource
	WAIT_OK = 17,
	SIGNAL_ERROR = 18,
	SIGNAL_OK = 19,
	FILE_NOT_OPEN = 20,	//File is not loaded in memory
	FILE_OPENED_BY_ANOTHER_PROCESS = 21,
	FILE_OPEN = 22,
	CLOSE_FILE = 23,
	KILL_PROCESS_CPU = 24,	//Tell the CPU to kill a process
	BLOCK_PROCESS_CPU = 25,	//Tell the CPU to block a process

	//DMA-Scheduler Tasks
	UNLOCK_PROCESS = 26,	//The DMA module informs the scheduler it finished sending a process' data to memory

	//Misc messages
	MESSAGE_RECEIVED = 27
};

enum exit_codes
{
	CONFIG_LOAD_ERROR = -10
};

enum filePermissions
{
	READ = 1,
	READ_WRITE = 2,
	WRITE = 3
};

enum errorCodes
{
	MALLOC_ERROR = -9,		//Malloc/Calloc returned a NULL pointer
	CONFIG_PROPERTY_NOT_FOUND = -10
};

#endif /* ENUMS_H_ */
