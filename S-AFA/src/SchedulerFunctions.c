#include "Scheduler.h"


config_t getConfigs()
{
	char *configurationPath = calloc(35, sizeof(char));
	strcpy(configurationPath, "../../Configs/Scheduler.cfg"); //strcpy stops copying characters when it encounters a '\0', memcpy stops when it copies the defined amount

	t_config* configFile = config_create(configurationPath);

	config_t config;


	if(config_has_property(config, "AlgoritmoPlanificacion"))
	{
		config.schedulingAlgorithm = strdup(config_get_string_value(config, "AlgoritmoPlanificacion"));

		if(strcmp(config.schedulingAlgorithm, "RR") == 0)
			config.schedulingAlgorithmCode = RR;

		else if(strcmp(config.schedulingAlgorithm, "VRR") == 0)
			config.schedulingAlgorithmCode = VRR;

		else if(strcmp(config.schedulingAlgorithm, "Custom") == 0)
			config.schedulingAlgorithmCode = Custom;
	}
	else
	{
		log_error(schedulerLog, "No se definio ninguna propiedad para el 'Algoritmo de Planificacion' en el archivo de configuracion\nAbortando el proceso...\n");
		free(configurationPath);
		config_destroy(configFile);
		exit(ExitCode_CONFIGNOTFOUND); //Property_Missing error code
	}

	if(config_has_property(configFile, "NivelMultiprogramacion"))
	{
		config.multiprogrammingLevel = config_get_int_value(configFile, "NivelMultiprogramacion");
	}
	else
	{
		log_error(schedulerLog, "No se definio ninguna propiedad para el 'Nivel de Multiprogramacion' en el archivo de configuracion\nAbortando el proceso...\n");
		free(configurationPath);
		config_destroy(configFile);
		exit(ExitCode_CONFIGNOTFOUND);
	}

	if(config_has_property(configFile, "Puerto"))
	{
		config.port = config_get_string_value(configFile, "Puerto");
	}
	else
	{
		log_error(schedulerLog, "No  se  definio ninguna propiedad para el 'Puerto' en el archivo de configuracion\nAbortando el proceso...\n");
		free(configurationPath);
		config_destroy(configFile);
		exit(ExitCode_CONFIGNOTFOUND);
	}

	if(config_has_property(configFile, "Quantum"))
	{
		config.port = config_get_string_value(configFile, "Quantum");
	}
	else
	{
		log_error(schedulerLog, "No se definio ninguna propiedad para el 'Quantum' en el archivo de configuracion\nAbortando el proceso...\n");
		free(configurationPath);
		config_destroy(configFile);
		exit(ExitCode_CONFIGNOTFOUND);
	}

	if(config_has_property(configFile, "RetardoPlanificacion"))
	{
		config.port = config_get_string_value(configFile, "RetardoPlanificacion");
	}
	else
	{
		log_error(schedulerLog, "No se definio ninguna propiedad para el 'Retardo de Planificacion' en el archivo de configuracion\nAbortando el proceso...\n");
		free(configurationPath);
		config_destroy(configFile);
		exit(ExitCode_CONFIGNOTFOUND);
	}

	free(configurationPath);
	config_destroy(configFile);
	return config;
}

void checkAlgorithm()
{
	switch(config.schedulingAlgorithmCode)
	{
		case RR:
			scheduleProcesses = &roundRobinScheduler;
			break;


		case VRR:
			scheduleProcesses = &virtualRoundRobinScheduler;
			break;

		case Custom:
			scheduleProcesses = &customScheduler;
			break;

		default:
			log_warning(schedulerLog, "Se especifico un algoritmo incorrecto en el archivo de configuracio\nSe utilizara el algoritmo de planificacion FIFO\n");
			scheduleProcesses = &fifoScheduler;
			break;
	}
}

void addProcessToNewQueue(PCB_t *process)
{
	uint32_t semaphoreValue;

	process->executionState = NEW;
	process->newQueueArrivalTime = executedInstructions;
	list_add(newQueue, process);

	log_info(schedulerLog, "Se creo un proceso con id %d!\n", process->pid);

	/*
	sem_getvalue(&longTermScheduler, &semaphoreValue);

	if((semaphoreValue == 0) && !LTSAlreadyExecuting) //Semaphore may be 0, but the scheduler may or may not be running
		sem_post(&longTermScheduler);
	*/

	sem_post(&longTermScheduler);

	// If a process gets added to the new queue, just signal the LTS semaphore, no need to
	// sleep the LTS when it starts. It does not matter if you pause scheduling, the
	// first time the LTS starts it gets all the processes, and then it runs several times
	// after that without adding processes (and it prints messages telling it has no new processes)
}

uint32_t addProcessToReadyQueue(PCB_t *process)
{
	if((process->executionState == NEW))
	{
		if(activeProcesses == config.multiprogrammingLevel)
		{
			log_info(schedulerLog, "El proceso con id %i no pudo ser agregado a la cola de listos ya que se alcanzo el nivel de multiprogramacion\n", process->pid);
			//Send message to ESI telling it that the process could not be added
			return 0;
		}
		else
			activeProcesses++;

		process->newQueueLeaveTime = executedInstructions;
	}

	process->executionState = READY;
	list_add(readyQueue, process);

	log_info(schedulerLog, "El proceso con id %d fue aceptado en la cola de listos!\n", process->pid);

	return 1;
}

void moveProcessToReadyQueue(PCB_t* process, bool addToIoReadyQueue) //To add to the real ReadyQueue, or the idReadyQueue
{
	process->executionState = READY;

	if(addToIoReadyQueue)
	{
		list_add(readyQueue, process);
		log_info(schedulerLog, "El proceso con id %d fue movido a la cola de listos\n", process->pid);
	}
	else
	{
		list_add(ioReadyQueue, process);
		log_info(schedulerLog, "El proceso con id %d fue movido a la cola auxiliar de listos\n", process->pid);
	}

}

void addProcessToBlockedQueue(PCB_t* process)
{
	process->executionState = BLOCKED;
	list_add(blockedQueue, process);
}

PCB_t* createProcess(char* scriptName)
{
	PCB_t* process = calloc(1, sizeof(PCB_t));

	if(process == NULL)	//no se pudo reservar memoria
		return NULL;

									
		process->newQueueArrivalTime = executedInstructions;
	process->newQueueLeaveTime = 0;
	process->pid = ++totalProcesses;
	process->programCounter = NULL;
	process->scriptName = strdup(scriptName); // scriptName es parte de una estructura que va a ser liberada
	process->wasInitialized = false;
	process->canBeScheduled = false;
	process->executionState = SYSTEM_PROCESS;
	process->cpuProcessIsAssignedTo = 0;
	process->readyQueueArrivalTime = 0;
	process->dmaCalls = 0;
	process->instructionsExecuted = 0;
	process->firstExecutionTime = 0;
	process->normalTermination = true;

	return process;
}

void terminateExecutingProcess(uint32_t processId)
{
	bool _process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == processId;
	}

	uint32_t semaphoreValue;
	PCB_t* process = list_remove_by_condition(executionQueue, _process_has_given_id);

	process->executionState = TERMINATED;

	list_add(finishedQueue, process);
										  

	activeProcesses--;

	/*
	sem_getvalue(&longTermScheduler, &semaphoreValue);

	if((semaphoreValue == 0) && !LTSAlreadyExecuting) //Semaphore may be 0, but the scheduler may or may not be running
		sem_post(&longTermScheduler); //Always let the LTS accept new processes before running the STS
	*/

	sem_post(&longTermScheduler); //Post the LITS semaphore; it does not matter if it got
								  //posted several times by new processes, and it runs several times


void unblockProcess(uint32_t processId)
{
	bool _process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == processId;
	}

	PCB_t* process = list_remove_by_condition(blockedQueue, _process_has_given_id);

	process->wasInitialized = true;	//If the process was not initialized and the DMA told me to unblock it, its initialization completed succesfully
	//Need to do something else?

	if(config.schedulingAlgorithmCode == VRR)
		moveProcessToReadyQueue(process, true);
	else
		moveProcessToReadyQueue(process, false);
}

uint32_t getFreeCPUsQty()
{
	return list_size(getFreeCPUs());
}


t_list* getFreeCPUs()
{
	uint32_t cpuListSize = list_size(connectedCPUs);
	t_list* freeCPUs = list_create();
	cpu_t* cpu;

	for(uint32_t i = 0; i < cpuListSize; i++)
	{
		cpu = list_get(connectedCPUs, i);

		if(cpu->isFree)
			list_add(freeCPUs, cpu);
	}

	return freeCPUs;
}

											 
 
	   
 

//TODO: If a new CPU connects, check if there are any ready processes and schedule
//		them to the new CPU