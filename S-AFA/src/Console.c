#include "Scheduler.h"

void console()
{
	char* line = NULL;
	char* token = NULL;
	console_t* console = NULL;

	printf("\n--------CONSOLE COMMANDS--------\n");
	printf("\n\tejecutar <scriptPath> - Permite ejecutar un script. Se debe proporcionar la ruta del script dentro del FileSystem FIFA\n");
	printf("\n\tstatus <processId> - Existen 2 opciones para este comando. Se le puede proporcionar el ID de un proceso, para lo cual proporciona informacion sobre las variables internas del mismo; o se puede ejecutar sin pasarle parametros, para lo cual proporciona informacion sobre las colas de planificacion\n");
	printf("\n\tfinalizar <processId> - Permite finalizar de forma prematura la ejecucion de un proceso\n");
	printf("\n\tmetricas <processId> - Existen 2 opciones para este comando. Se le puede proporcionar el ID de un proceso, para lo cual se muestran metricas relativas al mismo; o se puede ejecutar sin pasarle parametros, para lo cual muestra metricas relativas al sistema\n");
	printf("\n\tpausar - Permite detener la planificacion de procesos\n");
	printf("\n\tcontinuar - Permite reanudar la planificacion de procesos\n");
	printf("\n\tsalir - Permite cerrar este modulo\n\n");
	printf("\nNOTA: Para el comando finalizar, debito a la falta de tiempo y conocimientos para realizar una implementacion apropiada, no es posible matar procesos que esten siendo inicializados por una CPU\n");
	printf("-----------------------------------\n\n");

	//Lo indicado en esa nota es asi porque implicaria romper un flujo de mensajes entre modulos, lo cual seria dificil de implementar
	//(ya que cada modulo, una vez que recibe una tarea, debe respetar un cierto flujo de mensajes (recepcion y envio))

	while (!terminateModule)
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
				string_to_lower(console->command);

				while (console->paramsQty < MAX_PARAMS && (token = strtok(NULL, " ")) != NULL)
					console->param[console->paramsQty++] = strdup(token);

				if (str_eq(console->command, "clear"))
					system("clear");

				else if (str_eq(console->command, "ejecutar"))
				{
					if (console->paramsQty < 1)
						print_c(consoleLog, "%s: falta la ruta del script que se desea ejecutar", console->command);
					else
					{
						char* script = console->param[0];
						executeScript(script);
					}
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
						log_info(consoleLog, "%s: numero incorrecto de argumentos. Recuerde que puede ingresar solo un id de proceso o ningun argumento", console->command);
				}
				else if (str_eq(console->command, "finalizar"))
				{
					if (console->paramsQty < 1)
						print_c(consoleLog, "%s: debe indicar el id de un proceso", console->command);
					else
					{
						uint32_t param = (uint32_t) atoi(console->param[0]);
						terminateProcessConsole(param);
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
						log_info(consoleLog, "%s: numero incorrecto de argumentos. Recuerde que puede ingresar solo un id de proceso o ningun argumento", console->command);
				}
				else if(str_eq(console->command, "salir"))
				{
					terminateModule = true;
					//Should send a message to all the CPUs and the DMA to tell them this process will exit, if there are any modules connected to this one
				}
				else if(str_eq(console->command, "continuar"))
				{
					//TODO - Usar lo hecho en el tp anterior
				}
				else if(str_eq(console->command, "pausar"))
				{
					//TODO - Usar lo hecho en el tp anterior
				}
				else
					print_c(consoleLog, "%s: Comando incorrecto", console->command);

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
		log_error(schedulerLog, "Ocurrio un problema al intentar crear el PCB para el nuevo proceso");
		return;
	}

	addProcessToNewQueue(process);
}

void getProcessMetrics(uint32_t processId)
{
	//Lock all queues, so results do not change during this functions execution
	pthread_mutex_lock(&readyQueueMutex);
	pthread_mutex_lock(&ioReadyQueueMutex);
	pthread_mutex_lock(&blockedQueueMutex);
	pthread_mutex_lock(&executionQueueMutex);
	pthread_mutex_lock(&finishedQueueMutex);

	char* queueName = "";
	PCB_t* process = getProcessFromSchedulingQueues(processId, queueName);	//Looks for the process with the given pid in the read, blocked, execution & finished queues

	if(process != NULL)
	{
		uint32_t newQueueWaitTime = process->newQueueLeaveTime - process->newQueueLeaveTime;
		uint32_t dmaInstructionsPercent = (process->completedDmaCalls * 100) / process->totalInstructionsExecuted;

		char* message = "\nMetricas para el proceso %d de la cola %s:\n\tCantidad de sentencias ejecutadas que paso el proceso en la cola NEW: %d\n\tPorcentaje de instrucciones del proceso que fueron al DMA: %d%%\n";

		log_info(consoleLog, message, process->pid, queueName, newQueueWaitTime, dmaInstructionsPercent);

	}

	pthread_mutex_unlock(&readyQueueMutex);
	pthread_mutex_unlock(&ioReadyQueueMutex);
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
	//Lock all queues, so results do not change during this functions execution
	pthread_mutex_lock(&readyQueueMutex);
	pthread_mutex_lock(&ioReadyQueueMutex);
	pthread_mutex_lock(&blockedQueueMutex);
	pthread_mutex_lock(&executionQueueMutex);
	pthread_mutex_lock(&finishedQueueMutex);

	char *queueName = NULL;
	char *scriptName = NULL, *wasInitialized = NULL, *canBeScheduled = NULL;
	uint32_t pid, programCounter, cpuProcessIsAssignedTo, remainingQuantum, newQueueArrivalTime;
	uint32_t newQueueLeaveTime, instructionsExecuted, completedDmaCalls, lastIOStartTime, responseTimes;
	uint32_t instructionsExecutedOnLastExecution, instructionsUntilIoOrEnd;

	PCB_t* process = getProcessFromSchedulingQueues(processId, queueName); //queueName == executionState

	if(process != NULL)
	{
		char* message = calloc(1, 357 * sizeof(char));
		strcpy(message, "\n-------------------Valores de las variables del PCB con id %d-------------------\n\n\t");
		strcat(message, "scriptName: %s\n\tprogramCounter: %d\n\twasInitialized: %s\n\tcanBeScheduled: %s\n\t");
		strcat(message, "executionState: %s\n\tcpuProcessIsAssignedTo: %d\n\tremainingQuantum: %d\n\t");
		strcat(message, "newQueueArrivalTime: %d\n\tnewQueueLeaveTime: %d\n\tinstructionsExecuted: %d\n\tinstructionsExecutedOnLastExecution: %d\n\t");
		strcat(message, "instructionsUntilIoOrEnd: %d\n\tcompletedDmaCalls: %d\n\tlastIOStartTime: %d\n\tresponseTimes: %d\n");

		pid = process->pid;
		programCounter = process->programCounter;
		cpuProcessIsAssignedTo = process->cpuProcessIsAssignedTo;
		remainingQuantum = process->remainingQuantum;
		newQueueArrivalTime = process->newQueueArrivalTime;
		newQueueLeaveTime = process->newQueueLeaveTime;
		instructionsExecuted = process->totalInstructionsExecuted;
		instructionsExecutedOnLastExecution = process->instructionsExecutedOnLastExecution;
		instructionsUntilIoOrEnd = process->instructionsUntilIoOrEnd;
		completedDmaCalls = process->completedDmaCalls;
		lastIOStartTime = process->lastIOStartTime;
		responseTimes = process->responseTimes;
		scriptName = process->scriptPathInFS;

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
				 newQueueLeaveTime, instructionsExecuted, instructionsExecutedOnLastExecution,
				 instructionsUntilIoOrEnd, completedDmaCalls, lastIOStartTime, responseTimes);

		free(message);
	}

	pthread_mutex_unlock(&readyQueueMutex);
	pthread_mutex_unlock(&ioReadyQueueMutex);
	pthread_mutex_unlock(&blockedQueueMutex);
	pthread_mutex_unlock(&executionQueueMutex);
	pthread_mutex_unlock(&finishedQueueMutex);
}

void getQueuesStatus()
{
	pthread_mutex_lock(&readyQueueMutex);
	pthread_mutex_lock(&ioReadyQueueMutex);
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
	pthread_mutex_unlock(&ioReadyQueueMutex);
	pthread_mutex_unlock(&blockedQueueMutex);
	pthread_mutex_unlock(&executionQueueMutex);
	pthread_mutex_unlock(&finishedQueueMutex);
}

void terminateProcessConsole(uint32_t processId)
{
	int32_t nbytes;
	uint32_t _socket;
	cpu_t* cpu = NULL;

	bool cpu_is_executing_given_process(cpu_t* cpu)
	{
		return cpu->currentProcess == processId;
	}

	pthread_mutex_lock(&cpuListMutex);

	cpu = (cpu_t*) list_find(connectedCPUs, cpu_is_executing_given_process);
	_socket = cpu->serverSocket;

	pthread_mutex_unlock(&cpuListMutex);


	if(cpu != NULL)	//If there is a CPU executing the given process, tell it to issue a kill request (so the CPU can finish properly)
	{
		if((nbytes = send_int_with_delay(cpu->serverSocket, KILL_PROCESS_CPU)) < 0)
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
	//No need to block all the queues, they are blocked by the functions that call this one

	PCB_t* process = NULL;
	t_list* queueToSearch = NULL;

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
