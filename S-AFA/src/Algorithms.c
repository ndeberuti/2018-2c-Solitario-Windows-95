#include "Scheduler.h"


void roundRobinScheduler()
{
	PCB_t* scheduledProcess;
	t_list* freeCPUs = getFreeCPUs();
	cpu_t* selectedCPU;

	if(list_size(freeCPUs) == 0)
	{
		log_info(schedulerLog, "No hay CPUs disponibles para ejecutar el proceso planificado\n");
		return;
	}

	scheduledProcess = list_remove_by_condition(readyQueue, processCanBeScheduled); //The ready queue has processes which cannot be scheduled (script not yet loaded in memory)
	selectedCPU = list_get(freeCPUs, 0);

	dispatchProcess(scheduledProcess, selectedCPU);

}

void virtualRoundRobinScheduler(PCB_t* process)
{
	PCB_t* scheduledProcess;
	t_list* freeCPUs = getFreeCPUs();
	cpu_t* selectedCPU;

	if(list_size(freeCPUs) == 0)
	{
		log_info(schedulerLog, "No hay CPUs disponibles para ejecutar el proceso planificado\n");
		return;
	}

	if(list_size(ioReadyQueue) != 0)
		scheduledProcess = list_remove(ioReadyQueue,  0);
	else
		scheduledProcess = list_remove(readyQueue, 0);

	selectedCPU = list_get(freeCPUs, 0);

	dispatchProcess(scheduledProcess, selectedCPU);
}

void customScheduler()
{
	//TODO
}


void dispatchProcess(PCB_t* process, cpu_t* selectedCPU)
{
	selectedCPU->currentProcess = process->pid;
	selectedCPU->isFree = false;

	process->cpuProcessIsAssignedTo = selectedCPU->cpuId;

	if(process->executionState == READY)	//Modify the quantum only if it comes from the readyQueue, not from the ioReadyQueue
		process->remainingQuantum =  config.quantum;

	process->executionState = EXECUTING;

	list_add(executionQueue, process);
	sendPCB(process, selectedCPU->socket);

	log_info(schedulerLog, "El proceso con id %d fue seleccionado para ejecutar en la CPU con id %d\n", process->pid,  selectedCPU->cpuId);
}

bool processCanBeScheduled(PCB_t* pcb)
{
	return pcb->canBeScheduled;
}

//TODO: When I receive the PCB back from the CPU, I have to remove the one with the same pid
//		from the executionQueue

//For the RR schedulers, the CPU should decrement the PCB quantum for each instruction executed, unless it calls the DMA
//In that case, the CPU sends back the PCB to this process... then the process is put in the blocked queue and, when
//it exists the blocked queue, it is put in the ioReadyQueue.. the process' remaining quantum should never be modified
//unless it goes from the readyQueue to the executionQueue


