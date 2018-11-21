#include "Scheduler.h"


void console()
{
	char* line;
	char* token;
	console_t* console;

	while (true)
	{
		line = readline("S-AFA> ");

		if (strlen(line) > 0)
		{
			add_history(line);
			log_info(consoleLog, "Linea leida: %s", line);
			console = malloc(sizeof(console_t));

			if (console != NULL)
			{
				console->command = strdup(strtok(line, " "));
				console->paramsQty = 0;

				while (console->paramsQty < MAX_PARAMS && (token = strtok(NULL, " ")) != NULL)
					console->param[console->paramsQty++] = strdup(token);

				if (str_eq(console->command, "clear"))
					system("clear");

				else if (str_eq(console->command, "ejecutar"))
				{
					if (console->paramsQty < 1)
						print_c(consoleLog, "%s: falta la ruta del script que se desea ejecutar\n", console->command);
					else
						executeScript(console->param[0]);
				}
				else if (str_eq(console->command, "status"))
				{
					if (console->paramsQty == 1)
					{
						uint32_t param = (uint32_t) atoi(console->param[0]);
						getProcessStatus(param);
					}
					else if (console->paramsQty == 0)
						getQueuesStatus();
					else
						log_info(consoleLog, "%s: numero incorrecto de argumentos. Recuerde que puede ingresar solo un id de proceso o ningun argumento\n", console->command);
				}
				else if (str_eq(console->command, "finalizar"))
				{
					if (console->paramsQty < 1)
						print_c(consoleLog, "%s: debe indicar el id de un proceso\n", console->command);
					else
					{
						uint32_t param = (uint32_t) atoi(console->param[0]);
						terminateProcess(param);
					}
				}
				else if (str_eq(console->command, "metricas"))
				{
					if (console->paramsQty == 1)
					{
						uint32_t param = (uint32_t) atoi(console->param[0]);
						getProcessMetrics(param);
					}
					else if (console->paramsQty == 0)
						getSystemMetrics();
					else
						log_info(consoleLog, "%s: numero incorrecto de argumentos. Recuerde que puede ingresar solo un id de proceso o ningun argumento\n", console->command);
				}
				else
					print_c(consoleLog, "%s: Comando incorrecto\n", console->command);

				free(console->command);
				for (uint32_t i = 0; i < console->paramsQty; i++)
					free(console->param[i]);

				free(console);
			}
		}

		free(line);
	}
}

void executeScript(char* script)
{
	PCB_t* process = createProcess(script);

	if(process == NULL)
	{
		log_error(schedulerLog, "Ocurrio un problema al intentar crear el PCB para el nuevo proceso\n");
		return;
	}

	pthread_mutex_lock(&newQueueMutex);

	addProcessToNewQueue(process);

	pthread_mutex_unlock(&newQueueMutex);
}

void getProcessMetrics(uint32_t processId)
{
	//Lock all queues, so results do not change during this functions execution
	pthread_mutex_lock(&readyQueueMutex);
	pthread_mutex_lock(&blockedQueueMutex);
	pthread_mutex_lock(&executionQueueMutex);
	pthread_mutex_lock(&finishedQueueMutex);

	char* queueName = "";
	PCB_t* process = getProcessFromSchedulingQueues(processId, queueName);	//Looks for the process with the given pid in the read, blocked, execution & finished queues

	if(process != NULL)
	{
		uint32_t newQueueWaitTime = process->newQueueLeaveTime - process->newQueueLeaveTime;
		uint32_t dmaInstructionsPercent = (process->completedDmaCalls * 100) / process->instructionsExecuted;

		char* message = "\nMetricas para el proceso %d de la cola %s:\n\tCantidad de sentencias ejecutadas que paso el proceso en la cola NEW: %d\n\tPorcentaje de instrucciones del proceso que fueron al DMA: %d%%\n";

		log_info(consoleLog, message, process->pid, queueName, newQueueWaitTime, dmaInstructionsPercent);

	}

	pthread_mutex_unlock(&readyQueueMutex);
	pthread_mutex_unlock(&blockedQueueMutex);
	pthread_mutex_unlock(&executionQueueMutex);
	pthread_mutex_unlock(&finishedQueueMutex);
}

void getSystemMetrics()
{
	pthread_mutex_lock(&metricsGlobalvariablesMutex);

	char* message = "\nMetricas del sistema:\n\tSentencias ejecutadas promedio que invocaron al DMA: %.2f\n\tSentencias promedio que provocaron la terminacion de un proceso: %.2f\n\tTiempo de respuesta promedio del sistema: %.2f\n";

	double medianDMACalls = ((double) dma_executedInstructions) / ((double)executedInstructions);
	double medianKillInstructions = ((double)killProcessInstructions) / ((double) executedInstructions);
	double medianSystemResponseTime = ((double) systemResponseTime) / ((double) dma_executedInstructions);

	log_info(consoleLog, message, medianDMACalls, medianKillInstructions, medianSystemResponseTime);

	pthread_mutex_unlock(&metricsGlobalvariablesMutex);
}

void getProcessStatus(uint32_t processId)
{
	char *queueName = NULL;
	char *scriptName, *wasInitialized, *canBeScheduled;
	uint32_t pid, programCounter, cpuProcessIsAssignedTo, remainingQuantum, newQueueArrivalTime;
	uint32_t newQueueLeaveTime, instructionsExecuted, completedDmaCalls, lastIOStartTime, responseTimes;

	PCB_t* process = getProcessFromSchedulingQueues(processId, queueName); //queueName == executionState

	if(process != NULL)
	{
		char* message = calloc(1, 357 * sizeof(char));
		strcpy(message, "\nValores de las variables del PCB con id %d:\n\t");
		strcat(message, "scriptName: %s\n\tprogramCounter: %d\n\twasInitialized: %s\n\tcanBeScheduled: %s\n\t");
		strcat(message, "executionState: %s\n\tcpuProcessIsAssignedTo: %d\n\tremainingQuantum: %d\n\t");
		strcat(message, "newQueueArrivalTime: %d\n\tnewQueueLeaveTime: %d\n\tinstructionsExecuted: %d\n\t");
		strcat(message, "completedDmaCalls: %d\n\tlastIOStartTime: %d\n\tresponseTimes: %d\n");

		pid = process->pid;
		programCounter = process->programCounter;
		cpuProcessIsAssignedTo = process->cpuProcessIsAssignedTo;
		remainingQuantum = process->remainingQuantum;
		newQueueArrivalTime = process->newQueueArrivalTime;
		newQueueLeaveTime = process->newQueueLeaveTime;
		instructionsExecuted = process->instructionsExecuted;
		completedDmaCalls = process->completedDmaCalls;
		lastIOStartTime = process->lastIOStartTime;
		responseTimes = process->responseTimes;
		scriptName = process->scriptName;

		if(process->wasInitialized)
			wasInitialized = "TRUE";
		else
			wasInitialized = "FALSE";

		if(process->canBeScheduled)
			canBeScheduled = "TRUE";
		else
			canBeScheduled = "FALSE";

		log_info(consoleLog, message, pid, scriptName, programCounter, wasInitialized, canBeScheduled,
				 queueName, cpuProcessIsAssignedTo, remainingQuantum, newQueueArrivalTime,
				 newQueueLeaveTime, instructionsExecuted, completedDmaCalls, lastIOStartTime, responseTimes);

		free(message);
	}
}

void getQueuesStatus()
{
	pthread_mutex_lock(&readyQueueMutex);
	pthread_mutex_lock(&blockedQueueMutex);
	pthread_mutex_lock(&executionQueueMutex);
	pthread_mutex_lock(&finishedQueueMutex);

	char* message = calloc(1, 513 * sizeof(char));
	char *queueName = NULL;
	char *readyProcesses = calloc(1, 64 * sizeof(char));
	char *blockedProcesses = calloc(1, 64 * sizeof(char));
	char *executingProcesses = calloc(1, 64 * sizeof(char));
	char *finishedProcesses = calloc(1, 64 * sizeof(char));
	char *readyIoProcesses = calloc(1, 64 * sizeof(char));
	char *stringToReplace = NULL;
	char *pidStr = NULL;
	t_list* queueToSearch = NULL;
	t_list* mappedList = NULL;
	uint32_t stringLength = 0;

	char* get_pid_string_from_pcb(PCB_t* pcb)
	{
		char* pidString = calloc(1, 3 * sizeof(char));	//Don't think process IDs will go above 2 digits
		sprintf(pidString, "%d", pcb->pid);
		return pidString;
	}

	strcpy(message, "\nInformacion de estado de las colas de planificacion:\n");
	uint32_t messageLength = 56;

	for(uint32_t i = 0; i < 5; i++)
	{
		switch(i)
		{
			case 0:
				queueToSearch = readyQueue;
				queueName = "READY";
				stringToReplace = readyProcesses;
			break;

			case 1:
				queueToSearch = blockedQueue;
				queueName = "BLOCK";
				stringToReplace = blockedProcesses;
			break;

			case 2:
				queueToSearch = executionQueue;
				queueName = "EXEC";
				stringToReplace = executingProcesses;
			break;

			case 3:
				queueToSearch = finishedQueue;
				queueName = "FINISH";
				stringToReplace = finishedProcesses;
			break;

			case 4:
				queueToSearch = ioReadyQueue;
				queueName = "READY_IO";
				stringToReplace = readyIoProcesses;
			break;
		}

		//If I put the last "%%s" with only one "%", C would look for a string that is not present
		//in the arguments I passed to the "sprintf" function (after the "queueName" variable)
		//so, to be able to use that "%s" later in another format function, it must be printed like that
		//in the desired string. To make it so C prints "%s" without looking for a second string
		//argument after "queueName", I have to put double "%" signs, so C prints a single one
		//("%%" is a escape char for the "%" format sign)
		//So, the first "%s" gets replaced by the string in "queueName" and the "%%s" get replaced
		//by "%s", so it can be used in another format function

		messageLength += sprintf(message + messageLength, "\tProcesos en la cola %s:%%s\n", queueName);

		mappedList = list_map(queueToSearch, get_pid_string_from_pcb);
		stringLength = 0;

		for(uint32_t j = 0; j < list_size(mappedList); j++)
		{
			pidStr = (char*) list_get(mappedList, j);

			//Using "sprintf" to be able to get the length of the "stringToReplace" after adding each pid
			stringLength += sprintf(stringToReplace + stringLength, " %s,", pidStr);
		}

		sprintf(stringToReplace + stringLength, "\b"); //To remove the last comma
		list_destroy_and_destroy_elements(mappedList, free);
	}

	log_info(consoleLog, message, readyProcesses, blockedProcesses, executingProcesses,
			 finishedProcesses, readyIoProcesses);

	log_info(consoleLog, "Para obtener informacion de cada proceso debe ejecutar el comando \"status <pid>\"...\n");


	free(message);
	free(readyProcesses);
	free(executingProcesses);
	free(blockedProcesses);
	free(finishedProcesses);
	free(readyIoProcesses);

	pthread_mutex_unlock(&readyQueueMutex);
	pthread_mutex_unlock(&blockedQueueMutex);
	pthread_mutex_unlock(&executionQueueMutex);
	pthread_mutex_unlock(&finishedQueueMutex);
}

void terminateProcess(uint32_t processId)
{
	int32_t nbytes;

	bool cpu_is_executing_given_process(cpu_t* cpu)
	{
		return cpu->currentProcess == processId;
	}

	cpu_t* cpu = (cpu_t*) list_find(connectedCPUs, cpu_is_executing_given_process);

	if(cpu != NULL)	//If there is a CPU executing the given process, tell it to issue a kill request (so the CPU can finish properly)
	{
		if((nbytes = send_int_with_delay(cpu->socket, KILL_PROCESS_CPU)) < 0)
		{
			log_error(consoleLog, "Console - Error al indicar a la CPU que debe terminar un proceso\n");
			return;
			//TODO (Optional) - Send Error Handling
		}
	}
	else
		killProcess(processId);
}

PCB_t* getProcessFromSchedulingQueues(uint32_t processId, char* queueName)
{
	PCB_t* process = NULL;
	t_list* queueToSearch;

	bool process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == processId;
	}

	for(uint32_t i = 0; ((i < 5) && (process == NULL)); i++)
	{
		switch(i)
		{
			case 0:
				queueToSearch = readyQueue;
				queueName = "READY";
			break;

			case 1:
				queueToSearch = blockedQueue;
				queueName = "BLOCK";
			break;

			case 2:
				queueToSearch = executionQueue;
				queueName = "EXEC";
			break;

			case 3:
				queueToSearch = finishedQueue;
				queueName = "FINISH";
			break;

			case 4:
				queueToSearch = ioReadyQueue;
				queueName = "READY_IO";
			break;
		}

		//If no processes match the given id, the following function returns NULL
		process = list_find(queueToSearch, process_has_given_id);
	}

	if(process == NULL)	//The specified process is not in any of the queues
		log_info(schedulerLog, "El proceso indicado no existe o aun no fue aceptado (esta en la cola NEW)\n");

	return process;
}
