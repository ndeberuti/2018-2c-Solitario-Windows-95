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

	checkAlgorithm();

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
}

void addProcessToNewQueue(PCB_t *process)	//This function is used to move uninitialized processes from the newQueue to the readyQueue
{
	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	process->executionState = NEW;
	process->newQueueArrivalTime = executedInstructions;

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);


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

	// If a process gets added to the new queue, just signal the LTS semaphore, no need to
	// sleep the LTS when it starts. It does not matter if you pause scheduling, the
	// first time the LTS starts it gets all the processes, and then it runs several times
	// after that without adding processes (and it prints messages telling it has no new processes)
}

uint32_t addProcessToReadyQueue(PCB_t *process)
{
	pthread_mutex_lock(&configFileMutex);
		uint32_t multiprrogrammingLevel = config.multiprogrammingLevel;
	pthread_mutex_unlock(&configFileMutex);

	//No need to lock the readyQueue with a mutex, it was already locked in the LTS that called this function

	if((process->executionState == NEW))
	{
		pthread_mutex_lock(&metricsGlobalvariablesMutex);

		if(activeProcesses == multiprrogrammingLevel)
		{
			log_info(schedulerLog, "El proceso con id %i no pudo ser agregado a la cola de listos ya que se alcanzo el nivel de multiprogramacion", process->pid);

			pthread_mutex_unlock(&metricsGlobalvariablesMutex);
			return 0;
		}
		else
			activeProcesses++;

		pthread_mutex_unlock(&metricsGlobalvariablesMutex);

		process->newQueueLeaveTime = executedInstructions;
	}

	process->executionState = READY;
	list_add(readyQueue, process);

	log_info(schedulerLog, "El proceso con id %d fue aceptado en la cola de listos!", process->pid);

	return 1;
}

void moveProcessToReadyQueue(PCB_t* process, bool addToIoReadyQueue) //To add to the real ReadyQueue, or the idReadyQueue
{
	process->executionState = READY;
	process->cpuProcessIsAssignedTo = 0;

	if(!addToIoReadyQueue)
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

	//Need to post the STS each time a process is added to the readyQueue, so it can be scheduled...but only post when a initialized process is added
	//(otherwise the STS will have nothing to schedule). It has to be posted for each process, because the algorithms always take the first process
	//of the readyQueue; the others are left untouched. If there are no free CPUs, the STS will not be able to schedule processes, and
	//will inform the user about it
	//If I try to post the STS only from the LTS, when a uninitialized process is moved to the ready queue, then this module will never schedule processes
	if(process->wasInitialized)
	{
		processesReadyToExecute++;
		sem_post(&shortTermScheduler);
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
	PCB_t* process = calloc(1, sizeof(PCB_t));

	if(process == NULL)	//no se pudo reservar memoria
		return NULL;


	pthread_mutex_lock(&metricsGlobalvariablesMutex);
									
	process->newQueueArrivalTime = executedInstructions;
	process->pid = ++totalProcesses;

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);

	process->newQueueLeaveTime = 0;
	process->programCounter = 0;
	process->scriptPathInFS = string_duplicate(scriptPathInFS); // scriptName es parte de una estructura que va a ser liberada
	process->wasInitialized = false;
	process->executionState = SYSTEM_PROCESS;
	process->cpuProcessIsAssignedTo = 0;
	process->completedDmaCalls = 0;
	process->responseTimes = 0;
	process->lastIOStartTime = 0;
	process->totalInstructionsExecuted = 0;
	process->instructionsExecutedOnLastExecution = 0;
	process->instructionsUntilIoOrEnd = 0;

	return process;
}

void terminateExecutingProcess(PCB_t* process)
{
	process->executionState = TERMINATED;

	pthread_mutex_lock(&finishedQueueMutex);

	list_add(finishedQueue, process);

	pthread_mutex_unlock(&finishedQueueMutex);
										  

	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	activeProcesses--;

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);

	/*
	sem_getvalue(&longTermScheduler, &semaphoreValue);

	if((semaphoreValue == 0) && !LTSAlreadyExecuting) //Semaphore may be 0, but the scheduler may or may not be running
		sem_post(&longTermScheduler); //Always let the LTS accept new processes before running the STS
	*/

	checkAndFreeProcessFiles(process->pid);
	checkAndFreeProcessSemaphores(process->pid);

	log_info(schedulerLog, "El proceso con id %d fue movido a la cola de procesos terminados, ya que se terminaron las instruccciones de su script", process->pid);

	int32_t ltsSemaphore;
	sem_getvalue(&longTermScheduler, &ltsSemaphore);

	if((ltsSemaphore == 0) && (!list_is_empty(newQueue)))
		sem_post(&longTermScheduler); //If there are new processes waiting to be accepted, post the LTS semaphore; it does not matter if it got
										//posted several times by new processes and it runs several times
}

void unblockProcess(uint32_t processId, bool unblockedByDMA)
{
	pthread_mutex_lock(&configFileMutex);

	uint32_t schedulingAlgorithmCode = config.schedulingAlgorithmCode;

	pthread_mutex_unlock(&configFileMutex);


	PCB_t* process = NULL;
	process = removeProcessFromQueueWithId(processId, blockedQueue);

	if(process == NULL)
	{
		log_error(schedulerLog, "El proceso %d deberia estar en la cola de bloqueados, pero no se encuentra en ella, por lo que no deberia poder ser desbloqueado (y no deberiua haber llegado una llamada del DMA para desbloquearlo). Este modulo sera abortado para que evalue la situacion", processId);
		exit(EXIT_FAILURE);
	}

	log_info(schedulerLog, "El proceso %d fue quitado de la cola de bloqueados", processId);

	process->wasInitialized = true;	//If the process was not initialized and the DMA told me to unblock it, its initialization completed succesfully
	//Need to do something else?

	if(unblockedByDMA)
	{
		pthread_mutex_lock(&metricsGlobalvariablesMutex);

		uint32_t lastIOResponseTime = executedInstructions - (process->lastIOStartTime);
		systemResponseTime += lastIOResponseTime;
		dma_executedInstructions++;

		pthread_mutex_unlock(&metricsGlobalvariablesMutex);

		(process->completedDmaCalls)++;
		process->responseTimes += lastIOResponseTime;
		process->lastIOStartTime = 0;


		if(schedulingAlgorithmCode == VRR)
			moveProcessToReadyQueue(process, true);
		else
			moveProcessToReadyQueue(process, false);
	}
	else
		moveProcessToReadyQueue(process, false);
}

void blockProcess(PCB_t* process, bool isDmaCall)	//This is both used to block a process when a file is not open, when a semaphore has no instances, or when opening a file
{												    //That is why I dont pass it a pcb ()
	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	uint32_t _executedInstructions = executedInstructions;

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);

	process->executionState = BLOCKED;
	process->cpuProcessIsAssignedTo = 0;

	if(isDmaCall && (process->wasInitialized))
	{
		process->lastIOStartTime = _executedInstructions;
	}

	pthread_mutex_lock(&blockedQueueMutex);

	list_add(blockedQueue, process);

	pthread_mutex_unlock(&blockedQueueMutex);


	log_info(schedulerLog, "El proceso con id %d fue movido a la cola de bloqueados", process->pid);
}

uint32_t getFreeCPUsQty()
{
	t_list* freeCPUs = getFreeCPUs();
	uint32_t freeCPUsQty = list_size(freeCPUs);

	list_destroy(freeCPUs);

	return freeCPUsQty;
}


t_list* getFreeCPUs()
{
	t_list* freeCPUs = NULL;

	bool _cpu_is_free(cpu_t* cpu)
	{
		return cpu->isFree;
	}

	log_info(schedulerLog, "Buscando CPUs libres...");

	freeCPUs = list_filter(connectedCPUs, _cpu_is_free);

	if((freeCPUs != NULL) && (list_size(freeCPUs) > 0))
	{
		log_info(schedulerLog, "Se han encontrado CPUs libres! :D ");
	}
	else
	{
		log_info(schedulerLog, "No se han encontrado CPUs libres :( ...");
	}

	return freeCPUs;
}

void initializeOrExecuteProcess(PCB_t* process, cpu_t* selectedCPU)
{
	int32_t nbytes = 0;

	pthread_mutex_lock(&configFileMutex);

	uint32_t quantum = config.quantum;

	pthread_mutex_unlock(&configFileMutex);


	char* taskMessage = NULL;

	selectedCPU->currentProcess = process->pid;
	selectedCPU->isFree = false;

	process->cpuProcessIsAssignedTo = selectedCPU->cpuId;

	if(process->executionState == READY)	//Modify the quantum only if it comes from the readyQueue, not from the ioReadyQueue;
	{										//if the process was not initialized, this property will not be used by the cpu, but there is no need to change this for that case
		process->remainingQuantum =  quantum;
	}

	process->executionState = EXECUTING;

	pthread_mutex_lock(&executionQueueMutex);

	list_add(executionQueue, process);

	pthread_mutex_unlock(&executionQueueMutex);

	//As this is a request from the scheduler to the CPU, and to avoid communications mixing with the requests
	//made from the CPUs to the scheduler's serverThread, all the communication from this function with the CPUs is mathe through the CPUs serverThread

	if(!process->wasInitialized)
	{
		if((nbytes = send_int_with_delay(selectedCPU->clientSocket, INITIALIZE_PROCESS)) < 0)
		{
			log_error(consoleLog, "SchedulerFunctions (executeProcess) - Error al pedir a la CPU que inicialice un proceso");
			return;
			//TODO (Optional) - Send Error Handling
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

	log_info(schedulerLog, "El proceso con id %d fue enviado para %s en la CPU con id %d", process->pid, taskMessage,  selectedCPU->cpuId);
}

void killProcess(uint32_t pid)
{
	pthread_mutex_lock(&readyQueueMutex);
	pthread_mutex_lock(&blockedQueueMutex);
	pthread_mutex_lock(&executionQueueMutex);
	pthread_mutex_lock(&finishedQueueMutex);
	pthread_mutex_lock(&ioReadyQueueMutex);


	PCB_t* processToKill = NULL;
	t_list* queueToSearch = NULL;


	log_info(schedulerLog, "Se intentara terminar el proceso %d de forma prematura", pid);

	t_list* queues[] = {readyQueue, blockedQueue, executionQueue, finishedQueue, ioReadyQueue};	//An array with a pointer to each of the scheduling queues

	for(uint32_t i = 0; ((i < 5) && (processToKill == NULL)); i++)
	{
		queueToSearch = queues[i];

		//If no processes match the given id, the following function returns NULL
		processToKill = removeProcessFromQueueWithId(pid, queueToSearch);
	}

	if(processToKill == NULL)	//The process is not in any of the queues
	{
		log_error(consoleLog, "El proceso que se intento terminar no existe o ya fue terminado");

		pthread_mutex_unlock(&readyQueueMutex);
		pthread_mutex_unlock(&blockedQueueMutex);
		pthread_mutex_unlock(&executionQueueMutex);
		pthread_mutex_unlock(&finishedQueueMutex);
		pthread_mutex_unlock(&ioReadyQueueMutex);

		return;
	}

	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	killProcessInstructions++;

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);


	processToKill->executionState = TERMINATED;
	list_add(finishedQueue, processToKill);

	pthread_mutex_unlock(&readyQueueMutex);
	pthread_mutex_unlock(&blockedQueueMutex);
	pthread_mutex_unlock(&executionQueueMutex);
	pthread_mutex_unlock(&finishedQueueMutex);
	pthread_mutex_unlock(&ioReadyQueueMutex);


	checkAndFreeProcessFiles(pid);
	checkAndFreeProcessSemaphores(pid);

	log_info(schedulerLog, "El proceso %d fue terminado de forma exitosa, y los recursos que tenia tomados fueron liberados", pid);
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

	cpu_t* cpuToRemove = NULL;
	cpuToRemove = list_remove_by_condition(connectedCPUs, _cpu_has_given_socket);

	pthread_mutex_unlock(&cpuListMutex);

	log_info(schedulerLog, "La CPU con id %d fue quitada de la lista de CPUs y su socket sera cerrado, ya que la misma fue desconectada", cpuToRemove->cpuId);

	free(cpuToRemove);
	close(cpuSocket);
}
	   
void checkAndFreeProcessFiles(uint32_t processId)	//Checks if there are any files locked by the process and frees them
{
	//t_list* fileTableKeys = dictionary_get_keys(fileTable);
	uint32_t fileTableKeysQty = list_size(fileTableKeys);
	char* currentFile = NULL;
	fileTableData* data = NULL;
	fileTableData* dataToRemove = NULL;
	uint32_t processToUnblock = 0;
	t_list* processWaitList = NULL;
	uint32_t processesWaitingForFile = 0;

	pthread_mutex_lock(&fileTableMutex);

	for(uint32_t i = 0; i < fileTableKeysQty; i++)
	{
		currentFile = list_get(fileTableKeys, i);
		data = dictionary_get(fileTable, currentFile);

		if(data->processFileIsAssignedTo == processId)
		{
			//Unblock all of the processes that were waiting for the file, which was taken by the process that is being terminated
			//Then remove the file from the file table

			processWaitList = data->processesWaitingForFile;
			processesWaitingForFile = list_size(processWaitList);

			if(processesWaitingForFile == 0)
			{
				log_info(schedulerLog, "El archivo \"%s\", el cual habia sido tomado por el proceso %d (que esta siendo terminado), no tiene procesos en espera", currentFile, processId);
			}
			else
			{
				log_info(schedulerLog, "El archivo \"%s\", el cual habia sido tomado por el proceso %d (que esta siendo terminado), tiene procesos en espera", currentFile, processId);


				processToUnblock = (uint32_t) list_remove(processWaitList, 0);

				char* processId = string_itoa(processToUnblock);
				char* procWaitingForFileString = string_new();
				string_append(&procWaitingForFileString, processId);

				free(processId);

				for(uint32_t i = 1; i < processesWaitingForFile; i++)
				{
					processToUnblock = (uint32_t) list_remove(processWaitList, i);
					unblockProcess(processToUnblock, false);

					processId = string_from_format(", %d", processToUnblock);
					string_append(&procWaitingForFileString, processId);

					free(processId);
				}

				log_info(schedulerLog, "Los siguientes procesos esperaban por la liberacion del archivo \"%s\" y fueron desbloqueados: %s", currentFile, processesWaitingForFile);
				free(procWaitingForFileString);
			}

			dataToRemove = dictionary_remove(fileTable, currentFile);
			list_destroy(dataToRemove->processesWaitingForFile);
			free(dataToRemove);
		}
	}

	pthread_mutex_unlock(&fileTableMutex);
}

void checkAndFreeProcessSemaphores(uint32_t processId)
{
	pthread_mutex_lock(&semaphoreListKeysMutex);
	pthread_mutex_lock(&semaphoreListMutex);

	//t_list* semaphores = dictionary_get_keys(semaphoreList);
	uint32_t semaphoresQty = list_size(semaphoreListKeys);
	char* currentSemaphore = NULL;
	semaphoreData* data = NULL;
	uint32_t processToUnblock;
	t_list* processWaitList = NULL;
	t_list* usingProcessesList = NULL;
	bool processHasTakenSemaphore = false;
	bool processHasTakenAnySemaphore = false;


	bool _pid_equals_given_one(uint32_t pid)
	{
		return pid == processId;
	}


	for(uint32_t i = 0; i < semaphoresQty; i++)
	{
		currentSemaphore = list_get(semaphoreListKeys, i);
		data = dictionary_get(semaphoreList, currentSemaphore);
		processWaitList = data->waitingProcesses;
		usingProcessesList = data->processesUsingTheSemaphore;

		processHasTakenSemaphore = list_any_satisfy(usingProcessesList, _pid_equals_given_one);

		if(processHasTakenSemaphore)
		{
			processHasTakenAnySemaphore = true;

			list_remove_by_condition(usingProcessesList, _pid_equals_given_one);
			(data->semaphoreValue)++;

			log_info(schedulerLog, "El proceso %d habia tomado una instancia del semaforo \"%s\", la cual fue liberada", processId, currentSemaphore);

			if(list_size(processWaitList) > 0)
			{
				processToUnblock = (uint32_t) list_remove(processWaitList, 0);
				unblockProcess(processToUnblock, false);

				log_info(schedulerLog, "Debido a que un proceso libero una instancia del semaforo \"%s\", el proceso %d, el cual esperaba por dicho semaforo, fue desbloqueado", currentSemaphore, processToUnblock);
			}
		}
	}

	if(!processHasTakenAnySemaphore)
		log_info(schedulerLog, "El proceso %d no tenia semaforos tomados", processId);

	pthread_mutex_unlock(&semaphoreListKeysMutex);
	pthread_mutex_unlock(&semaphoreListMutex);
}

int32_t send_int_with_delay(uint32_t _socket, uint32_t messageCode)
{
	pthread_mutex_lock(&configFileMutex);
	uint32_t schedulingDelay = config.schedulingDelay;
	pthread_mutex_unlock(&configFileMutex);


	int32_t nbytes;

	log_info(schedulerLog, "Se comenzara el envio de un mensaje");

	double milisecondsSleep = schedulingDelay / 1000;

	log_info(schedulerLog, "Delay de envio de mensaje: %d ms", schedulingDelay);
	sleep(milisecondsSleep);

	nbytes = send_int(_socket, messageCode);

	log_info(schedulerLog, "Se ha finalizado el envio de un mensaje");

	return nbytes;
}

int32_t send_PCB_with_delay(PCB_t* pcb, uint32_t _socket)
{
	pthread_mutex_lock(&configFileMutex);
	uint32_t schedulingDelay = config.schedulingDelay;
	pthread_mutex_unlock(&configFileMutex);

	int32_t nbytes = 0;

	log_info(schedulerLog, "Se comenzara el envio de un PCB");

	double milisecondsSleep = schedulingDelay / 1000;

	log_info(schedulerLog, "Delay de envio de mensaje: %d ms", schedulingDelay);
	sleep(milisecondsSleep);

	nbytes = sendPCB(_socket, pcb);

	log_info(schedulerLog, "Se ha finalizado el envio de un PCB");

	return nbytes;
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
	//No need to add a mutex here, the readyQueue is locked by the functions that call this one

	log_info(schedulerLog, "Obteniendo procesos planificables");

	bool processCanBeScheduled(PCB_t* pcb)
	{
		return pcb->wasInitialized;
	}

	return list_filter(readyQueue, processCanBeScheduled);
}

void checkAndInitializeProcesses(cpu_t* freeCPU)
{
	//Takes any of the uninitialized processes from the readyQueue, and initializes it with the
	//given CPU (this function is called when a CPU connects or is freed)

	//No need to lock the connectedCPUs list here, it is already locked by the functions that call this one

	pthread_mutex_lock(&readyQueueMutex);	//This list works with a process from the readyQueue, so I need to lock that queue too to avoid the process being modified

	PCB_t* processToInitialize = NULL;
	t_list* uninitializedProcesses = NULL;
	uint32_t uninitializedProcessesQty = 0;


	bool _process_is_uninitialized(PCB_t* pcb)
	{
		return !(pcb->wasInitialized);
	}
/*
	bool _process_has_given_id(PCB_t* pcb)		//Cannot place this here because processToInitialize is null
	{
		return (pcb->pid == processToInitialize->pid);
	}
*/

	//The new list should have all the uninitialized processes ordered by the time they arrived
	//to the readyQueue
	uninitializedProcesses = list_filter(readyQueue, _process_is_uninitialized);
	uninitializedProcessesQty = list_size(uninitializedProcesses);

	if((uninitializedProcesses != NULL) && (uninitializedProcessesQty > 0))
	{
		log_info(schedulerLog, "Hay %d procesos sin inicializar. Debido a que hay una CPU libre, se inicializara el primer proceso sin inicializar de la cola de listos", uninitializedProcessesQty);
	}
	else
	{
		log_info(schedulerLog, "No hay procesos para inicializar");

		pthread_mutex_unlock(&readyQueueMutex);

		return;
	}

	log_info(schedulerLog, "");

	processToInitialize = list_get(uninitializedProcesses, 0);
	processToInitialize->wasInitialized = false;

	bool _process_has_given_id(PCB_t* pcb)	//As 'processToInitialize' is assigned above, I have to create this function here for it to work
	{
		return (pcb->pid == processToInitialize->pid);
	}

	log_info(schedulerLog, "Se inicializara el proceso con id %d", processToInitialize->pid);

	list_remove_by_condition(readyQueue, _process_has_given_id);

	initializeOrExecuteProcess(processToInitialize, freeCPU);

	log_info(schedulerLog, "LTS: El proceso %d fue enviado a una CPU para ser inicializado...", processToInitialize->pid);


	pthread_mutex_unlock(&readyQueueMutex);
}

void checkAndInitializeProcessesLoop()
{
	//Takes all the uninitialized processes from the readyQueue, and initializes them with the available CPUs (Used in the LTS thread)

	//No need to lock the connectedCPUs list, it is already locked in the LTS that calls this function


	PCB_t* processToInitialize = NULL;
	t_list* uninitializedProcesses = NULL;
	uint32_t uninitializedProcessesQty = 0;
	t_list* freeCPUs = getFreeCPUs();	//If there are no freeCPUs, this list is empty
	uint32_t freeCPUsQty = list_size(freeCPUs);
	cpu_t* freeCPU = NULL;


	bool _process_is_uninitialized(PCB_t* pcb)
	{
		return !(pcb->wasInitialized);
	}
/*
	bool _process_has_given_id(PCB_t* pcb)		//Cannot place this here because processToInitialize is null
	{
		return (pcb->pid == processToInitialize->pid);
	}
*/

	//There is no need to lock the readyQueue, because It already got locked by the LTS that called this function

	//The new list should have all the uninitialized processes ordered by the time they arrived
	//to the readyQueue
	uninitializedProcesses = list_filter(readyQueue, _process_is_uninitialized);
	uninitializedProcessesQty = list_size(uninitializedProcesses);

	if((uninitializedProcesses != NULL) && (uninitializedProcessesQty > 0))
	{
		log_info(schedulerLog, "Hay %d procesos sin inicializar. Se comenzara la inicializacion...", uninitializedProcessesQty);
	}
	else
	{
		log_info(schedulerLog, "No hay procesos para inicializar");

		if(freeCPUs != NULL)
			list_destroy(freeCPUs);

		return;
	}

	for(uint32_t i = 0; (i < uninitializedProcessesQty) && (freeCPUsQty > 0); i++)	//If the uninitializedProcesses list size is 0, this loop never starts; the same goes for the freeCPUs list
	{
		freeCPU = list_get(freeCPUs, 0);

		processToInitialize = list_get(uninitializedProcesses, i);
		processToInitialize->wasInitialized = false;

		bool _process_has_given_id(PCB_t* pcb)	//As 'processToInitialize' is assigned above, I have to create this function here for it to work
		{
			return (pcb->pid == processToInitialize->pid);
		}

		log_info(schedulerLog, "Se inicializara el proceso con id %d", processToInitialize->pid);

		list_remove_by_condition(readyQueue, _process_has_given_id);

		initializeOrExecuteProcess(processToInitialize, freeCPU);

		log_info(schedulerLog, "LTS: El proceso %d fue enviado a una CPU para ser inicializado...", processToInitialize->pid);

		if(freeCPUs != NULL)
			list_destroy(freeCPUs);		//Need to destroy the previous filtered list (list_filter always creates a new list) but, as
										//it contains pointers to the original CPUsList, this lists elements cannot be deleted
		freeCPUs = getFreeCPUs();
		freeCPUsQty = list_size(freeCPUs);

		if(freeCPUsQty == 0)
		{
			log_info(schedulerLog, "LTS: No hay mas CPUs disponibles para inicializar procesos, se continuara con la planificacion...");
		}
	}

	if(freeCPUs != NULL)
		list_destroy(freeCPUs);
}

PCB_t* removeProcessFromQueueWithId(uint32_t processId, t_list* queue)
{
	bool _process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == processId;
	}

	PCB_t* removedPCB = NULL;
	removedPCB = (PCB_t*) list_remove_by_condition(queue, _process_has_given_id);

	return removedPCB;
}

void killProcessWithPCB(PCB_t* process)
{
	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	killProcessInstructions++;

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);


	process->executionState = TERMINATED;
	list_add(finishedQueue, process);

	checkAndFreeProcessFiles(process->pid);
	checkAndFreeProcessSemaphores(process->pid);

	log_info(schedulerLog, "El proceso %d fue terminado de forma exitosa, y los recursos que tenia tomados fueron liberados", process->pid);
}
