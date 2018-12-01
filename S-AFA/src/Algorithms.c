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
	//TODO - Para hacerlo, deberia agregar al PCB un contador que guarda la cantidad de instrucciones
	//		 que faltan antes de una operacion de IO o el fin del script
	//		 la CPU tiene que verificar el contador cada vez que recibe el PCB. Si el contador
	//		 esta en 0, hay que parsear el script y contar la cantidad de instrucciones antes de
	//		 una de IO o del fin del script.
	//		 Despues, con cada instruccion ejecutada se decrementa el contador de instrucciones
	//		 restantes. Como la CPU parsea, para no tener que actualizar el PCB de este modulo
	//		 cada vez que recalcula ese contador, la CPU se encarga de ir descontando el contador
	//
	//		 Despues, para planificar, se ordena la cola de listos por ese contador, poniendo el
	//		 menor primero, y se elige el menor para ejecutar...
}

//For the RR schedulers, the CPU should decrement the PCB quantum for each instruction executed, unless it calls the DMA
//In that case, the CPU sends back the PCB to this module... then the process is put in the blocked queue or,
//if the algorithm is VRR, it is put in the ioReadyQueue.. the process' remaining quantum should never be modified
//unless it goes from the readyQueue to the executionQueue


