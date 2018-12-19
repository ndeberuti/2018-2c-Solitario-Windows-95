#include "Scheduler.h"

//Para cada uno de estos algoritmos por ahi hay que hacer un ciclo for y asignar
//procesos a cada cpu vacia; o por ahi tengo que hacer de a uno para evitar que se
//llenen todas las cpu y no haya ninguna disponible para inicializar procesos...

bool roundRobinScheduler()
{
	PCB_t* scheduledProcess = NULL;
	t_list* freeCPUs = getFreeCPUs();
	cpu_t* selectedCPU = NULL;

	bool processCanBeScheduled(PCB_t* pcb)
	{
		return pcb->wasInitialized;
	}

	if(list_is_empty(freeCPUs))
	{
		log_info(schedulerLog, "No hay CPUs disponibles para ejecutar el proceso planificado\n");
		return false;
	}

	//No need to add a mutex here.. this is used by the STS thread, and the readyQueue already got locked before entering here

	scheduledProcess = list_remove_by_condition(readyQueue, processCanBeScheduled); //The ready queue has processes which cannot be scheduled (script not yet loaded in memory)



	selectedCPU = (cpu_t*) list_get(freeCPUs, 0);



	initializeOrExecuteProcess(scheduledProcess, selectedCPU);

	list_destroy(freeCPUs);
	return true;
}


bool virtualRoundRobinScheduler()
{
	PCB_t* scheduledProcess = NULL;
	t_list* freeCPUs = getFreeCPUs();
	cpu_t* selectedCPU = NULL;

	bool processCanBeScheduled(PCB_t* pcb)
	{
		return pcb->wasInitialized;
	}

	if(list_is_empty(freeCPUs))
	{
		log_info(schedulerLog, "No hay CPUs disponibles para ejecutar el proceso planificado\n");
		return false;
	}

	if(list_size(ioReadyQueue) != 0)
	{
		//No need to add a mutex here.. this is used by the STS thread, and the ioReadyQueue
		//already got locked before entering here

		scheduledProcess = list_remove(ioReadyQueue,  0);
	}
	else
	{
		//No need to add a mutex here.. this is used by the STS thread, and the readyQueue
		//already got locked before entering here

		scheduledProcess = list_remove_by_condition(readyQueue, processCanBeScheduled);
	}

	selectedCPU = (cpu_t*) list_get(freeCPUs, 0);
	initializeOrExecuteProcess(scheduledProcess, selectedCPU);

	list_destroy(freeCPUs);
	return true;
}

bool customScheduler()
{
	//If the scheduler gets to this point, there is a free CPU
	PCB_t* scheduledProcess = NULL;
	t_list* freeCPUs = getFreeCPUs();
	cpu_t* selectedCPU = NULL;
	t_list* schedulableProcesses = NULL;
	t_list* processesToCountInstructions = NULL;
	uint32_t processesToCountInstructionsQty = 0;
	uint32_t instructionsUntilIO = 0;
	PCB_t* processToCountInstructions = NULL;


	bool processIOInstructionCounterIsZero(PCB_t* pcb)
	{
		return pcb->instructionsUntilIoOrEnd == 0;
	}

	bool processHasLessInstructionsUntilIO(PCB_t* pcb1, PCB_t* pcb2)
	{
		return pcb1->instructionsUntilIoOrEnd <= pcb2->instructionsUntilIoOrEnd;
	}


	if(list_is_empty(freeCPUs))
	{
		log_info(schedulerLog, "No hay CPUs disponibles para ejecutar el proceso planificado\n");
		return false;
	}

	selectedCPU = (cpu_t*) list_get(freeCPUs, 0);

	list_destroy(freeCPUs);

	//No need to add a mutex here for the readyQueue, cause it got locked by the STS that called this function
	schedulableProcesses = getSchedulableProcesses();

	if(list_is_empty(schedulableProcesses))
	{
		log_warning(schedulerLog, "STS: Se activo el planificador de corto plazo, pero no existen procesos planificables en la cola READY. Esto no deberia haber pasado\n");
		return false;
	}

	processesToCountInstructions = list_filter(schedulableProcesses, processIOInstructionCounterIsZero);
	processesToCountInstructionsQty = list_size(processesToCountInstructions);

	for(uint32_t i = 0; i < processesToCountInstructionsQty; i++)
	{
		processToCountInstructions = list_get(processesToCountInstructions, i);
		instructionsUntilIO = countProcessInstructions(processToCountInstructions, selectedCPU);

		if(instructionsUntilIO == -1)	//The CPU was disconnected, select a new one
		{
			closeSocketAndRemoveCPU(selectedCPU->clientSocket);

			freeCPUs = getFreeCPUs();

			selectedCPU = list_get(freeCPUs, 0);

			list_destroy(freeCPUs);
		}

		processToCountInstructions->instructionsUntilIoOrEnd = instructionsUntilIO;
	}

	list_sort(schedulableProcesses, processHasLessInstructionsUntilIO);
	scheduledProcess = list_get(schedulableProcesses, 0);	//The process with less instructions until the end or an IO instruction is scheduled
	initializeOrExecuteProcess(scheduledProcess, selectedCPU);

	list_destroy(processesToCountInstructions);

	return true;
}

uint32_t countProcessInstructions(PCB_t* process, cpu_t* selectedCPU)
{
	int32_t instructionsUntilIO = 0;
	int32_t nbytes = 0;
	uint32_t _socket = selectedCPU->clientSocket;	//As this is a request from the scheduler to a CPU, and to avoid its data mixing with
													//the one received by the scheduler's serverThread, this request is sent to the CPUs serverThread

	//To avoid sending all the PCB to the CPU, only certain variables are sent
	if((nbytes = send_int(_socket, COUNT_INSTRUCTIONS)) < 0)
	{
		log_error(consoleLog, "Algorithms (countProcessInstructions) - Error al indicar a la CPU que debe contar instrucciones para un proceso\n");
		return -1;
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(_socket, process->pid)) < 0)
	{
		log_error(consoleLog, "Algorithms (countProcessInstructions) - Error al enviar un pid a la CPU\n");
		return -1;
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_int(_socket, process->programCounter)) < 0)
	{
		log_error(consoleLog, "Algorithms (countProcessInstructions) - Error al enviar un program counter a la CPU\n");
		return -1;
		//TODO (Optional) - Send Error Handling
	}
	if((nbytes = send_string(_socket, process->scriptPathInFS)) < 0)
	{
		log_error(consoleLog, "Algorithms (countProcessInstructions) - Error al enviar un path de script a la CPU\n");
		return -1;
		//TODO (Optional) - Send Error Handling
	}

	//Receive the instructionsUntilIO count from the CPU
	if((nbytes = receive_int(_socket, &instructionsUntilIO)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "Algorithms (countProcessInstructions) - La CPU fue desconectada al intentar recibir la cantidad de instrucciones que le faltan a un proceso para una operacion de IO\n");
		else
			log_error(consoleLog, "Algorithms (countProcessInstructions) - Error al recibir la cantidad de instrucciones que le faltan a un proceso para una operacion de IO de la CPU\n");

		return -1;
	}

	return instructionsUntilIO;
}
