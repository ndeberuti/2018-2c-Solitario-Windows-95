#include "Scheduler.h"


int32_t getConfigs()
{
	char *configurationPath = calloc(35, sizeof(char));

	if(configurationPath == NULL)
		return MALLOC_ERROR;

	strcpy(configurationPath, "../../Configs/Scheduler.cfg"); //strcpy stops copying characters when it encounters a '\0', memcpy stops when it copies the defined amount

	t_config* configFile = config_create(configurationPath);
	config_t tempConfig;

	if(config_has_property(configFile, "AlgoritmoPlanificacion"))
	{
		tempConfig.schedulingAlgorithm = strdup(config_get_string_value(configFile, "AlgoritmoPlanificacion"));

		if(strcmp(tempConfig.schedulingAlgorithm, "RR") == 0)
			tempConfig.schedulingAlgorithmCode = RR;

		else if(strcmp(tempConfig.schedulingAlgorithm, "VRR") == 0)
			tempConfig.schedulingAlgorithmCode = VRR;

		else if(strcmp(tempConfig.schedulingAlgorithm, "Custom") == 0)
			tempConfig.schedulingAlgorithmCode = Custom;

		else
			tempConfig.schedulingAlgorithmCode = 0;
	}
	else
	{
		log_error(schedulerLog, "No se definio ninguna propiedad para el 'Algoritmo de Planificacion' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "NivelMultiprogramacion"))
	{
		tempConfig.multiprogrammingLevel = config_get_int_value(configFile, "NivelMultiprogramacion");
	}
	else
	{
		log_error(schedulerLog, "No se definio ninguna propiedad para el 'Nivel de Multiprogramacion' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "Puerto"))
	{
		tempConfig.port = config_get_int_value(configFile, "Puerto");
	}
	else
	{
		log_error(schedulerLog, "No se definio ninguna propiedad para el 'Puerto' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "Quantum"))
	{
		tempConfig.quantum = config_get_int_value(configFile, "Quantum");
	}
	else
	{
		log_error(schedulerLog, "No se definio ninguna propiedad para el 'Quantum' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	if(config_has_property(configFile, "RetardoPlanificacion"))
	{
		tempConfig.schedulingDelay = config_get_int_value(configFile, "RetardoPlanificacion");
	}
	else
	{
		log_error(schedulerLog, "No se definio ninguna propiedad para el 'Retardo de Planificacion' en el archivo de configuracion");
		free(configurationPath);
		config_destroy(configFile);
		return CONFIG_PROPERTY_NOT_FOUND;
	}

	free(configurationPath);
	config_destroy(configFile);

	config = tempConfig;	//This is to avoid modifying the original config values if an error occurs after modifying the configFile in runtime

	return 0;
}

void showConfigs()
{
	printf("\n");
	log_info(consoleLog, "---- Configuracion ----");
	log_info(consoleLog, "Puerto = %d", config.port);
	log_info(consoleLog, "Algoritmo de Planificacion = %s", config.schedulingAlgorithm);
	log_info(consoleLog, "Quantum = %d", config.quantum);
	log_info(consoleLog, "Nivel de Multiprogramacion = %d", config.multiprogrammingLevel);
	log_info(consoleLog, "Retardo de Planificacion = %d", config.schedulingDelay);
	log_info(consoleLog, "-----------------------");
	printf("\n");
}

void checkAlgorithm()
{
	pthread_mutex_lock(&configFileMutex);

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
			log_warning(schedulerLog, "Se especifico un algoritmo incorrecto en el archivo de configuracio\nSe utilizara el algoritmo de planificacion Round Robin");
			scheduleProcesses = &roundRobinScheduler;
			break;
	}

	pthread_mutex_unlock(&configFileMutex);
}

void addProcessToNewQueue(PCB_t *process)
{
	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	process->executionState = NEW;
	process->newQueueArrivalTime = executedInstructions;

	pthread_mutex_lock(&newQueueMutex);

	list_add(newQueue, process);

	pthread_mutex_unlock(&newQueueMutex);

	log_info(schedulerLog, "Se creo un proceso con id %d!", process->pid);

	/*
	sem_getvalue(&longTermScheduler, &semaphoreValue);

	if((semaphoreValue == 0) && !LTSAlreadyExecuting) //Semaphore may be 0, but the scheduler may or may not be running
		sem_post(&longTermScheduler);
	*/

	sem_post(&longTermScheduler);

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);

	// If a process gets added to the new queue, just signal the LTS semaphore, no need to
	// sleep the LTS when it starts. It does not matter if you pause scheduling, the
	// first time the LTS starts it gets all the processes, and then it runs several times
	// after that without adding processes (and it prints messages telling it has no new processes)
}

uint32_t addProcessToReadyQueue(PCB_t *process)
{
	pthread_mutex_lock(&configFileMutex);
	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	//No need to lock the readyQueue with a mutex, it was already locked in the LTS that called this function

	if((process->executionState == NEW))
	{
		if(activeProcesses == config.multiprogrammingLevel)
		{
			log_info(schedulerLog, "El proceso con id %i no pudo ser agregado a la cola de listos ya que se alcanzo el nivel de multiprogramacion", process->pid);

			pthread_mutex_unlock(&metricsGlobalvariablesMutex);
			return 0;
		}
		else
			activeProcesses++;

		process->newQueueLeaveTime = executedInstructions;
	}

	process->executionState = READY;
	list_add(readyQueue, process);

	log_info(schedulerLog, "El proceso con id %d fue aceptado en la cola de listos!", process->pid);

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);
	pthread_mutex_unlock(&configFileMutex);

	return 1;
}

void moveProcessToReadyQueue(PCB_t* process, bool addToIoReadyQueue) //To add to the real ReadyQueue, or the idReadyQueue
{
	process->executionState = READY;
	process->cpuProcessIsAssignedTo = 0;

	if(addToIoReadyQueue)
	{
		pthread_mutex_lock(&readyQueueMutex);

		list_add(readyQueue, process);

		pthread_mutex_unlock(&readyQueueMutex);

		log_info(schedulerLog, "El proceso con id %d fue movido a la cola de listos", process->pid);
	}
	else
	{
		pthread_mutex_lock(&ioReadyQueueMutex);

		list_add(ioReadyQueue, process);

		pthread_mutex_unlock(&ioReadyQueueMutex);

		log_info(schedulerLog, "El proceso con id %d fue movido a la cola auxiliar de listos", process->pid);
	}

}

void addProcessToBlockedQueue(PCB_t* process)
{
	process->executionState = BLOCKED;

	pthread_mutex_lock(&blockedQueueMutex);

	list_add(blockedQueue, process);

	pthread_mutex_unlock(&blockedQueueMutex);
}

PCB_t* createProcess(char* scriptPathInFS)
{
	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	PCB_t* process = calloc(1, sizeof(PCB_t));

	if(process == NULL)	//no se pudo reservar memoria
		return NULL;

									
	process->newQueueArrivalTime = executedInstructions;
	process->newQueueLeaveTime = 0;
	process->pid = ++totalProcesses;
	process->programCounter = 0;
	process->scriptPathInFS = string_duplicate(scriptPathInFS); // scriptName es parte de una estructura que va a ser liberada
	process->wasInitialized = false;
	process->canBeScheduled = false;
	process->executionState = SYSTEM_PROCESS;
	process->cpuProcessIsAssignedTo = 0;
	process->completedDmaCalls = 0;
	process->responseTimes = 0;
	process->lastIOStartTime = 0;
	process->totalInstructionsExecuted = 0;
	process->instructionsExecutedOnLastExecution = 0;
	process->instructionsUntilIoOrEnd = 0;

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);

	return process;
}

void terminateExecutingProcess(uint32_t processId)
{
	bool _process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == processId;
	}

	pthread_mutex_lock(&executionQueueMutex);

	PCB_t* process = list_remove_by_condition(executionQueue, _process_has_given_id);

	pthread_mutex_unlock(&executionQueueMutex);

	process->executionState = TERMINATED;

	pthread_mutex_lock(&finishedQueueMutex);

	list_add(finishedQueue, process);

	pthread_mutex_unlock(&finishedQueueMutex);
										  

	activeProcesses--;

	/*
	sem_getvalue(&longTermScheduler, &semaphoreValue);

	if((semaphoreValue == 0) && !LTSAlreadyExecuting) //Semaphore may be 0, but the scheduler may or may not be running
		sem_post(&longTermScheduler); //Always let the LTS accept new processes before running the STS
	*/

	checkAndFreeProcessFiles(processId);
	checkAndFreeProcessSemaphores(processId);

	log_info(schedulerLog, "El proceso con id %d fue movido a la cola FINISH, ya que se terminaron las instruccciones de su script", processId);

	sem_post(&longTermScheduler); //Post the LITS semaphore; it does not matter if it got
								  //posted several times by new processes and it runs several times
}

void unblockProcess(uint32_t processId, bool unblockedByDMA)
{
	pthread_mutex_lock(&configFileMutex);
	pthread_mutex_unlock(&metricsGlobalvariablesMutex);

	bool _process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == processId;
	}

	pthread_mutex_lock(&blockedQueueMutex);

	PCB_t* process = list_remove_by_condition(blockedQueue, _process_has_given_id);

	pthread_mutex_unlock(&blockedQueueMutex);

	process->wasInitialized = true;	//If the process was not initialized and the DMA told me to unblock it, its initialization completed succesfully
	//Need to do something else?

	if(unblockedByDMA)
	{
		(process->completedDmaCalls)++;
		uint32_t lastIOResponseTime = executedInstructions - (process->lastIOStartTime);
		process->responseTimes += lastIOResponseTime;
		process->lastIOStartTime = 0;

		systemResponseTime += lastIOResponseTime;
		dma_executedInstructions++;

		if(config.schedulingAlgorithmCode == VRR)
			moveProcessToReadyQueue(process, true);
		else
			moveProcessToReadyQueue(process, false);
	}
	else
		moveProcessToReadyQueue(process, false);

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);
	pthread_mutex_unlock(&configFileMutex);
}

void blockProcess(uint32_t pid, bool isDmaCall)
{
	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	bool _process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == pid;
	}

	PCB_t* process = list_remove_by_condition(executionQueue, _process_has_given_id);
	process->executionState = BLOCKED;
	process->cpuProcessIsAssignedTo = 0;

	if(isDmaCall && (process->wasInitialized))
	{
		process->lastIOStartTime = executedInstructions;
	}

	pthread_mutex_lock(&blockedQueueMutex);

	list_add(blockedQueue, process);

	pthread_mutex_unlock(&blockedQueueMutex);

	log_info(schedulerLog, "El proceso con id % fue movido a la cola de bloqueados", process->pid);

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);
}

uint32_t getFreeCPUsQty()
{
	return list_size(getFreeCPUs());
}


t_list* getFreeCPUs()
{
	pthread_mutex_lock(&cpuListMutex);

	bool _cpu_is_free(cpu_t* cpu)
	{
		return cpu->isFree;
	}

	t_list* freeCPUs = list_filter(connectedCPUs, _cpu_is_free);

	pthread_mutex_unlock(&cpuListMutex);

	return freeCPUs;
}

void executeProcess(PCB_t* process, cpu_t* selectedCPU)
{
	int32_t nbytes;
	int32_t message;

	pthread_mutex_lock(&configFileMutex);

	char* taskMessage = NULL;

	selectedCPU->currentProcess = process->pid;
	selectedCPU->isFree = false;

	process->cpuProcessIsAssignedTo = selectedCPU->cpuId;

	if(process->executionState == READY)	//Modify the quantum only if it comes from the readyQueue, not from the ioReadyQueue;
	{										//if the process was not initialized, this property will not be used by the cpu, but there is no need to change this for that case
		process->remainingQuantum =  config.quantum;
	}

	process->executionState = EXECUTING;

	pthread_mutex_lock(&executionQueueMutex);

	list_add(executionQueue, process);

	pthread_mutex_unlock(&executionQueueMutex);

	if(!process->wasInitialized)
	{
		if((nbytes = send_int_with_delay(selectedCPU->clientSocket, INITIALIZE_PROCESS)) < 0)
		{
			log_error(consoleLog, "executeProcess - Error al pedir a la CPU que inicialice un proceso");
			return;
			//TODO (Optional) - Send Error Handling
		}
		if((nbytes = receive_int(selectedCPU->clientSocket, &message)) <= 0)
		{
			if(nbytes == 0)
				log_error(consoleLog, "SchedulerFunctions (executeProcess) - La CPU fue desconectada al intentar recibir la confirmacion de recepcion de PCB");
			else
				log_error(consoleLog, "SchedulerFunctions (executeProcess) - Error al intentar recibir la confirmacion de recepcion de PCB de la CPU");

			closeSocketAndRemoveCPU(selectedCPU->clientSocket);
			FD_CLR(selectedCPU->clientSocket, &master);
		}

		taskMessage = "inicializar";
	}
	else
	{
		if((nbytes = send_int_with_delay(selectedCPU->clientSocket, EXECUTE_PROCESS)) < 0)
		{
			log_error(consoleLog, "executeProcess - Error al pedir a la CPU que ejecute un proceso");
			return;
			//TODO (Optional) - Send Error Handling
		}

		taskMessage = "ejecutar";
	}

	if((nbytes = send_PCB_with_delay(process, selectedCPU->clientSocket)) < 0)
	{
		log_error(consoleLog, "executeProcess - Error al pedir a la CPU que inicialice un archivo");
		return;
		//TODO (Optional) - Send Error Handling
	}

	//TODO - With send errors, which should appear if the selectedCPU disconnects or something like that, a new CPU should be selected, which has different id than the current one

	log_info(schedulerLog, "El proceso con id %d fue enviado para %s en la CPU con id %d", taskMessage, process->pid,  selectedCPU->cpuId);

	pthread_mutex_unlock(&configFileMutex);
}

void killProcess(uint32_t pid)
{
	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	PCB_t* processToKill = NULL;
	t_list* queueToSearch = NULL;


	bool process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == pid;
	}

	for(uint32_t i = 0; ((i < 5) && (processToKill == NULL)); i++)
	{
		switch(i)
		{
			case 0:
				queueToSearch = readyQueue;
			break;

			case 1:
				queueToSearch = blockedQueue;
			break;

			case 2:
				queueToSearch = executionQueue;
			break;

			case 3:
				queueToSearch = finishedQueue;
			break;

			case 4:
				queueToSearch = ioReadyQueue;
			break;
		}

			//If no processes match the given id, the following function returns NULL
			processToKill = list_remove_by_condition(queueToSearch, process_has_given_id);
	}

	if(processToKill == NULL)	//The process is not in any of the queues
	{
		log_error(consoleLog, "El proceso que se intento terminar no existe o ya fue terminado");
		pthread_mutex_unlock(&metricsGlobalvariablesMutex);
		return;
	}

	killProcessInstructions++;
	processToKill->executionState = TERMINATED;
	list_add(finishedQueue, processToKill);

	checkAndFreeProcessFiles(pid);
	checkAndFreeProcessSemaphores(pid);

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);
}

void closeSocketAndRemoveCPU(uint32_t cpuSocket)
{
	//Because i am finding cpus by sockets, if a cpu disconnects, I need to close the socket (that is being done)
	//so, when a socket gets closed, linux may use its fd number for another socket... by finding cpus by
	//sockets, if i do not remove a disconected cpu from the cpu list, it may happen that 2 o more cpus share
	//socket numbers; so if i am finding them by socket number (a file descriptor), and do not
	//cpus upon disconnection, i may get the worng cpu from the list (one that got disconnected)


	bool _cpu_has_given_socket(cpu_t* cpu)
	{
		return cpu->clientSocket == cpuSocket;
	}

	pthread_mutex_lock(&cpuListMutex);

	cpu_t* cpuToRemove = list_remove_by_condition(connectedCPUs, _cpu_has_given_socket);

	pthread_mutex_unlock(&cpuListMutex);

	free(cpuToRemove);
	close(cpuSocket);
}
	   
void checkAndFreeProcessFiles(uint32_t processId)	//Checks if there are any files locked by the process and frees them
{
	//t_list* fileTableKeys = dictionary_get_keys(fileTable);
	uint32_t fileTableKeysQty = list_size(fileTableKeys);
	char* currentKey;
	fileTableData* data;
	uint32_t processToUnblock;
	t_list* processWaitList;

	pthread_mutex_lock(&fileTableMutex);

	for(uint32_t i = 0; i < fileTableKeysQty; i++)
	{
		currentKey = list_get(fileTableKeys, i);
		data = dictionary_get(fileTable, currentKey);

		if(data->processFileIsAssignedTo == processId)
		{
			data->processFileIsAssignedTo = 0;
			processWaitList = data->processesWaitingForFile;

			if(list_size(processWaitList) > 0)
			{
				processToUnblock = (uint32_t) list_remove(processWaitList, 0);
				unblockProcess(processToUnblock, false);
			}
		}
	}

	pthread_mutex_unlock(&fileTableMutex);
}

void checkAndFreeProcessSemaphores(uint32_t processId)
{
	//t_list* semaphores = dictionary_get_keys(semaphoreList);
	uint32_t semaphoresQty = list_size(semaphoreListKeys);
	char* currentKey = NULL;
	semaphoreData* data;
	uint32_t processToUnblock;
	t_list* processWaitList = NULL;
	t_list* usingProcessesList = NULL;
	bool processHasTakenSemaphore;

	bool _pid_equals_given_one(uint32_t pid)
	{
		return pid == processId;
	}

	pthread_mutex_lock(&semaphoreListMutex);

	for(uint32_t i = 0; i < semaphoresQty; i++)
	{
		currentKey = list_get(semaphoreListKeys, i);
		data = dictionary_get(fileTable, currentKey);
		processWaitList = data->waitingProcesses;
		usingProcessesList = data->processesUsingTheSemaphore;

		processHasTakenSemaphore = list_any_satisfy(usingProcessesList, _pid_equals_given_one);

		if(processHasTakenSemaphore)
		{
			list_remove_by_condition(usingProcessesList, _pid_equals_given_one);
			(data->semaphoreValue)++;

			if(list_size(processWaitList) > 0)
			{
				processToUnblock = (uint32_t) list_remove(processWaitList, 0);
				unblockProcess(processToUnblock, false);
			}
		}
	}

	pthread_mutex_unlock(&semaphoreListMutex);
}

int32_t send_int_with_delay(uint32_t _socket, uint32_t messageCode)
{
	pthread_mutex_lock(&configFileMutex);

	int32_t nbytes;

	log_info(schedulerLog, "Se comenzara el envio de un mensaje...");

	double milisecondsSleep = config.schedulingDelay / 1000;

	log_info(schedulerLog, "Delay de envio de mensaje...\n");
	sleep(milisecondsSleep);

	nbytes = send_int(_socket, messageCode);

	log_info(schedulerLog, "Se ha finalizado el envio de un mensaje");

	pthread_mutex_unlock(&configFileMutex);

	return nbytes;
}

int32_t send_PCB_with_delay(PCB_t* pcb, uint32_t _socket)
{
	pthread_mutex_lock(&configFileMutex);

	int32_t nbytes;

	log_info(schedulerLog, "Se comenzara el envio de un mensaje...");

	double milisecondsSleep = config.schedulingDelay / 1000;

	log_info(schedulerLog, "Delay de envio de mensaje...\n");
	sleep(milisecondsSleep);

	nbytes = sendPCB(_socket, pcb);

	log_info(schedulerLog, "Se ha finalizado el envio de un mensaje");

	pthread_mutex_unlock(&configFileMutex);

	return nbytes;
}

void freeCpuElement(cpu_t* cpu)
{
	free(cpu->serverIp);
	free(cpu);
}

void freePCB(PCB_t* pcb)
{
	free(pcb->scriptPathInFS);
	free(pcb);
}

void freeFileTableData(fileTableData* data)
{
	list_destroy(data->processesWaitingForFile);
	free(data);
}

void freeSemaphoreListData(semaphoreData* data)
{
	list_destroy(data->processesUsingTheSemaphore);
	list_destroy(data->waitingProcesses);
	free(data);
}

void removeKeyFromList(t_list* table, char* key)
{
	bool _string_equals_given_one(char* string)
	{
		return string_equals_ignore_case(string, key);
	}

	list_remove_by_condition(table, _string_equals_given_one);
}

t_list* getSchedulableProcesses()
{
	bool processCanBeScheduled(PCB_t* pcb)
	{
		return pcb->canBeScheduled;
	}

	return list_filter(readyQueue, processCanBeScheduled);
}
