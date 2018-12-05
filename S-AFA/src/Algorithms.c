#include "Scheduler.h"

//Para cada uno de estos algoritmos por ahi hay que hacer un ciclo for y asignar
//procesos a cada cpu vacia; o por ahi tengo que hacer de a uno para evitar que se
//llenen todas las cpu y no haya ninguna disponible para inicializar procesos...

void roundRobinScheduler()
{
	PCB_t* scheduledProcess;
	t_list* freeCPUs = getFreeCPUs();
	cpu_t* selectedCPU;

	bool processCanBeScheduled(PCB_t* pcb)
	{
		return pcb->canBeScheduled;
	}

	if(list_size(freeCPUs) == 0)
	{
		log_info(schedulerLog, "No hay CPUs disponibles para ejecutar el proceso planificado\n");
		return;
	}

	//No need to add a mutex here.. this is used by the STS thread, and the readyQueue already got locked before entering here

	scheduledProcess = list_remove_by_condition(readyQueue, processCanBeScheduled); //The ready queue has processes which cannot be scheduled (script not yet loaded in memory)

	selectedCPU = list_get(freeCPUs, 0);
	executeProcess(scheduledProcess, selectedCPU);
}

void virtualRoundRobinScheduler(PCB_t* process)
{
	PCB_t* scheduledProcess;
	t_list* freeCPUs = getFreeCPUs();
	cpu_t* selectedCPU;

	bool processCanBeScheduled(PCB_t* pcb)
	{
		return pcb->canBeScheduled;
	}

	if(list_size(freeCPUs) == 0)
	{
		log_info(schedulerLog, "No hay CPUs disponibles para ejecutar el proceso planificado\n");
		return;
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

	selectedCPU = list_get(freeCPUs, 0);

	executeProcess(scheduledProcess, selectedCPU);
}

void customScheduler()
{
	//If the scheduler gets to this point, there is a free CPU
	PCB_t* scheduledProcess;
	t_list* freeCPUs = getFreeCPUs();
	cpu_t* selectedCPU;
	t_list* schedulableProcesses;
	t_list* processesToCountInstructions;
	uint32_t processesToCountInstructionsQty, instructionsUntilIO;
	PCB_t* processToCountInstructions;


	bool processIOInstructionCounterIsZero(PCB_t* pcb)
	{
		return pcb->instructionsUntilIoOrEnd == 0;
	}

	bool processHasLessInstructionsUntilIO(PCB_t* pcb1, PCB_t* pcb2)
	{
		return pcb1->instructionsUntilIoOrEnd <= pcb2->instructionsUntilIoOrEnd;
	}


	if(list_size(freeCPUs) == 0)
	{
		log_info(schedulerLog, "No hay CPUs disponibles para ejecutar el proceso planificado\n");
		return;
	}

	selectedCPU = list_get(freeCPUs, 0);

	schedulableProcesses = getSchedulableProcesses();

	if(list_size(schedulableProcesses) == 0)
	{
		log_warning(schedulerLog, "Se activo el planificador de corto plazo, pero no existen procesos planificables en la cola READY. Esto no deberia haber pasado\n");
	}

	processesToCountInstructions = list_filter(schedulableProcesses, processIOInstructionCounterIsZero);
	processesToCountInstructionsQty = list_size(processesToCountInstructions);

	if(processesToCountInstructionsQty > 0)
	{
		for(uint32_t i = 0; i < processesToCountInstructionsQty; i++)
		{
			processToCountInstructions = list_get(processesToCountInstructions, i);

			instructionsUntilIO = countProcessInstructions(processToCountInstructions, selectedCPU);

			if(instructionsUntilIO == -1)	//The CPU was disconnected, select a new one
			{
				closeSocketAndRemoveCPU(selectedCPU->clientSocket);

				freeCPUs = getFreeCPUs();

				if(list_size(freeCPUs) == 0)
				{
					log_warning(schedulerLog, "Se desconecto la ultima CPU libre al intentar planificar un proceso. No es posible planificar mas procesos hasta que haya mas CPUs libres\n");
					return;
				}

				selectedCPU = list_get(freeCPUs, 0);
			}

			processToCountInstructions->instructionsUntilIoOrEnd = instructionsUntilIO;
		}
	}

	list_sort(schedulableProcesses, processHasLessInstructionsUntilIO);

	scheduledProcess = list_get(schedulableProcesses, 0);	//The process with less instructions until the end or an IO instruction is scheduled

	executeProcess(scheduledProcess, selectedCPU);
}

uint32_t countProcessInstructions(PCB_t* process, cpu_t* selectedCPU)
{
	uint32_t instructionsUntilIO = 0;
	int32_t nbytes;
	uint32_t _socket = selectedCPU->clientSocket;

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
