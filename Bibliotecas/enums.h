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

	//CPU-Scheduler
	COMPLETED_INSTRUCTION = 6,	//Used by the scheduler to help keep track of how many instructions were executed at a given time (useful to get metrics)
	BLOCK_PROCESS_INIT = 7,		//This task is used when I need to receive only the process id (not the whole PCB) from the CPU
	BLOCK_PROCESS = 8,
	PROCESS_ERROR = 9, 			//Sent by the CPU & DMA; due to invalid FS/memory accesses
								//Or not enough memory space to load files/scripts; the Process is killed
	QUANTUM_END = 10,
	INITIALIZE_PROCESS = 11,	//The scheduler tells the CPU to initialize a process
	EXECUTE_PROCESS = 12,		//The scheduler tells the CPU to execute a process
	CHECK_IF_FILE_OPEN = 13,	//The CPU asks the scheduler to see if a file is open for a process
	WAIT_RESOURCE = 14,			//Wait on a semaphore
	SIGNAL_RESOURCE = 15,		//Signal a semaphore
	WAIT_ERROR = 16,			//The scheduler tells the CPU it could not reserve the specified semaphore
	WAIT_OK = 17,
	SIGNAL_ERROR = 18,			//The scheduler tells the CPU there was an error when trying to signal the specified semaphore
	SIGNAL_OK = 19,
	FILE_NOT_OPEN = 20,			//File is not loaded in memory
	FILE_OPENED_BY_ANOTHER_PROCESS = 21,
	FILE_OPEN = 22,
	CLOSE_FILE = 23,			//The CPU asks the scheduler or the memory to close a file (in the case of the memory, it should free all the data related to that file)
	KILL_PROCESS_CPU = 24,		//Tell the CPU to kill a process
	BLOCK_PROCESS_CPU = 25,		//Tell the CPU to block a process
	END_OF_SCRIPT = 26,
	USE_CUSTOM_ALGORITHM = 27,	//Used to tell all the CPUs that the scheduling algorithm changed to the custom one
	USE_NORMAL_ALGORITHM = 28,	//Used to tell all the cPUs that the scheduling algorithm changed to the normal ones (RR/VRR)
	COUNT_INSTRUCTIONS = 29,

	//DMA-Scheduler
	UNLOCK_PROCESS = 30,		//The DMA module informs the scheduler it finished sending a file's data to memory, or
								//it finished flushing a file's data from memory to the FS, or it finished creating a file for the specified process

	//CPU-DMA
	OPEN_FILE = 31,				//The CPU tells the DMA to open a file (take it from the specified path in the FS and send its data to memory)
	FLUSH_FILE = 32, 			//The CPU tells the DMA to send a file's data in memory to the FS
	CREATE_FILE = 33,			//The CPU tells the DMA to create a file
	DELETE_FILE = 34,			//The CPU tells the DMA to delete a file

	//CPU-Memory
	CLOSE_FILE_ERROR = 35,
	CLOSE_FILE_OK = 36,
	MODIFY_FILE = 37,			//The CPU tells the memory to assign data to a line of a file (it replaces the data of that line in the file with the provided one)
	MODIFY_FILE_ERROR = 38,
	MODIFY_FILE_OK = 39,
	REQUEST_SCRIPT = 40,

	//Misc messages
	MESSAGE_RECEIVED = 41,

	//DMA-FS
	OPERACION_FAIL = 42,
	BLOQUES_INSUFICIENTES = 43,
	OPERACION_OK = 44,
	OPERACION_ERROR = 45,
	VALIDAR_ARCHIVO = 46,
	CREAR_ARCHIVO = 47,
	OBTENER_DATOS = 48,
	GUARDAR_DATOS = 49,
	BORRAR_ARCHIVO = 50,
	UNKNOWN_FILE_SIZE = -1,

	//DMA-Memory
	CARGAR_ARCHIVO = 51,
	ASIGNAR = 52,
	FLUSH = 53,
	CLOSE_PROCESS = 55,
	LEER_ARCHIVO = 56,
	OK = 57,
	ESPACIO_INSUFICIENTE = 58,
	ARCHIVO_NO_ABIERTO = 59,
	PROCESO_NO_ABIERTO = 60
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
