#define LIM_INF_TAREAS_CPU 6
#define LIM_SUP_TAREAS_CPU 29
#define LIM_INF_TAREAS_DMA 30
#define LIM_SUP_TAREAS_DMA 30

#include "Scheduler.h"

void server()
{
	fd_set read_fds; //Temporal fd set for the 'select()' function
	struct sockaddr_in remoteaddr; //Client address
	uint32_t fdmax = 0; //Number of the max fd I got
	uint32_t newfd = 0; //Accepted connection _socket
	int32_t command = 0; //Client command
	uint32_t nbytes = 0;
	uint32_t addrlen = 0;
	int32_t result = 0;
	FD_ZERO(&master); //Delete master & read sets
	FD_ZERO(&read_fds);

	//Get listener _socket
	pthread_mutex_lock(&configFileMutex);

	uint32_t servidor = build_server(config.port, consoleLog);

	pthread_mutex_unlock(&configFileMutex);

	//Adding listener _socket to the master set
	FD_SET(servidor, &master);
	FD_SET(configFileInotifyFD, &master);

	if(servidor >= configFileInotifyFD)	//Check if the max fd number is assigned to the Inotify watch or to the server _socket
		fdmax = servidor;
	else
		fdmax = configFileInotifyFD;

	struct timeval selectTimeout;
	selectTimeout.tv_sec = 5;	//The select returns due to a timeout every 5 seconds
	selectTimeout.tv_usec = 0;

	//Main loop
	while (!terminateModule)
	{
		read_fds = master; //Copy the master set

		pthread_mutex_lock(&canCommunicateWithCPUs);

		if((result = select(fdmax + 1, &read_fds, NULL, NULL, &selectTimeout)) < 0)
		{
			log_error(consoleLog, "Error en select");
			exit(EXIT_FAILURE);
		}
		else if(result == 0)	//The select returned due to a timeout
		{
			pthread_mutex_unlock(&canCommunicateWithCPUs);
			sleep(0.5);	//To allow other threads to take the above lock
			continue;
		}


		//Check existent connections to see if there is any data to read
		for (uint32_t i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &read_fds))
			{ //Received data!
				if (i == servidor)
				{
					//Manage new connections
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(servidor, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
						log_error(consoleLog, "Error en accept");
					else
					{
						FD_SET(newfd, &master); //Add the _socket to the master set
						if (newfd > fdmax) //Update the max fd
							fdmax = newfd;
					}
				}
				else if(i == configFileInotifyFD)
				{
					handleConfigFileChanged();
				}
				else
				{
					//Manage data from a client
					if ((nbytes = receive_int(i, &command)) <= 0)
					{
						//Error or connection closed by client
						if (nbytes == 0)
						{
							//Connection closed
							if (i == dmaSocket)
								log_info(consoleLog, "DMA desconectado");
							else
								log_info(consoleLog, "CPU desconectada");
						}
						else
							log_error(consoleLog, "recv (comando)\n");

						close(i);
						FD_CLR(i, &master); //Remove _socket from master set
					}
					else
					{
						//Received data from a client
						moduleHandler(command, i);
					}
				}
			} // if (FD_ISSET(i, &read_fds))
		}

		pthread_mutex_unlock(&canCommunicateWithCPUs);	//Only after executing a CPUs request can the other threads communicate with that or other CPUs
		sleep(0.5);	//To allow other threads to take the above lock

	} // while (true)
}

void moduleHandler(uint32_t command, uint32_t _socket)
{
	if(command == NEW_DMA_CONNECTION)
	{
		log_info(consoleLog, "Nueva conexion desde el DMA");
		dmaSocket = _socket;
	}
	else if(command == NEW_CPU_CONNECTION)
		handleCpuConnection(_socket);
	else if((command >= LIM_INF_TAREAS_CPU) && (command <= LIM_SUP_TAREAS_CPU)) //The CPU task codes range from 3 to 12
	{
		log_info(schedulerLog, "Se recibio una tarea de la CPU");
		cpuTaskHandler(command, _socket);
	}

	else if((command >= LIM_INF_TAREAS_DMA) && (command <= LIM_SUP_TAREAS_DMA)) //The DMA task codes range from 13 to 18
	{
		log_info(schedulerLog, "Se recibio una tarea del DMA");
		dmaTaskHandler(command, _socket);
	}

	else
		log_error(consoleLog, "ServerThread - La tarea recibida, con codigo %d, es incorrecta", command);
}

void dmaTaskHandler(uint32_t task, uint32_t _socket)
{
	switch(task)
	{
		case UNLOCK_PROCESS:
		{
			unlockProcess(_socket);
		}
		break;

		default:
			log_error(consoleLog, "ServerThread - Se recibio una tarea incorrecta del DMA (codigo = %d)", task);
		break;
	}
}

void cpuTaskHandler(uint32_t task, uint32_t _socket)
{
	uint32_t process = 0;

	switch(task)
	{
		case COMPLETED_INSTRUCTION:
			log_info(schedulerLog, "Se completo una instruccion de un proceso en ejecucion");

			pthread_mutex_lock(&metricsGlobalvariablesMutex);
			executedInstructions++;
			pthread_mutex_unlock(&metricsGlobalvariablesMutex);
		break;

		case BLOCK_PROCESS_INIT:
			blockProcessInit(_socket, &process);
		break;

		case BLOCK_PROCESS:
			_blockProcess(_socket, &process);
		break;

		case PROCESS_ERROR:		//TODO - Maybe it should receive the type of error and print it in the console...
								//This PROCESS_ERROR message could be sent by the CPU or the DMA
			_killProcess(_socket);
		break;

		case QUANTUM_END:
			processQuantumEnd(_socket, &process);
		break;

		case CHECK_IF_FILE_OPEN:		//Checks if the given file is open for the issuing process, and tries to open it
			checkIfFileOpen(_socket);
		break;

		case CLOSE_FILE:
			closeFile(_socket);
		break;

		case WAIT_RESOURCE:		//Tries to wait on a semaphore and tells the CPU if it was possible
			waitResource(_socket);
		break;

		case SIGNAL_RESOURCE:		//Tries to signal a semaphore and tells the CPU if it was possible
			signalResource(_socket);
		break;

		case END_OF_SCRIPT:
			terminateProcess(_socket);
		break;

		default:
			log_error(consoleLog, "ServerThread - Se recibio una tarea incorrecta de la CPU (codigo = %d)", task);
		break;

	}

	if((task == BLOCK_PROCESS) || (task == BLOCK_PROCESS_INIT) || (task == QUANTUM_END))	//If any of this happened, it means a process left a CPU
	{																						//In that case, check if that process must be killed
		pthread_mutex_lock(&executingProcessesToKillMutex);

		uint32_t processesToKill = list_size(executingProcessesToKill);

		bool _process_and_socket_equal_current_ones(processToKillData* data)
		{
			return ((data->processToBeKilled == process) && (data->cpuSocketProcessWasExecutingOn == _socket));
		}

		bool currentProcessHasToBeKilled = list_any_satisfy(executingProcessesToKill, _process_and_socket_equal_current_ones);

		if((processesToKill > 0) && currentProcessHasToBeKilled)
		{
			killProcess(process);
			list_remove_by_condition(executingProcessesToKill, _process_and_socket_equal_current_ones);
		}

		pthread_mutex_unlock(&executingProcessesToKillMutex);
	}
}

void blockProcessInit(uint32_t _socket, uint32_t* process)
{
	int32_t pid = 0;
	int32_t nbytes = 0;	//This cannot be unsigned; check the recv below

	if((nbytes = receive_int(_socket, &pid)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (blockProcessInit) - La CPU fue desconectada al intentar recibir un pid");
		else
			log_error(consoleLog, "ServerThread (blockProcessInit) - Error al recibir un pid de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);

		//TODO (optional):
		//		 If a CPU disconnects while executing a process, it should tell all the other
		//		 modules and they should roll back all the changes that were made for that process
		//		 that was executing in the dead CPU.
		//		 Something like that should happen with all the other modules...
		//		 To achieve this, a module should have a signal catch (to avoid it closing without notice)
		//		 and when a module knows it will die (or another module died), it should tell all
		//		 the other modules connected to it...

		//TODO - If a cpu disconnects and doesn't tell this module to block the process after trying
		//to initialize it, the PCB will stay in the execution queue and when the DMA tells this module to
		//unlock that process, it will not be found in the blocked queue and, therefore, it will not be moved to the
		//ready queue properly....that should be fixed, either by telling the dma to dismiss that process
		//(and remove its script from memory), or by receiving the dma call and moving the process
		//fro the execution queue to the ready one

	}
	else
	{
		log_info(schedulerLog, "La CPU solicito que el proceso con id %d sea bloqueado luego de intentar inicializarlo", pid);

		PCB_t* processToBlock = NULL;
		processToBlock = removeProcessFromQueueWithId(pid, executionQueue);

		if(processToBlock == NULL)
		{
			log_error(schedulerLog, "El proceso %d deberia estar en la cola de ejecucion, pero no se encuentra en ella, por lo que no deberia poder ser bloqueado. Este modulo sera abortado para que evalue la situacion", pid);
			exit(EXIT_FAILURE);
		}

		log_info(schedulerLog, "El proceso %d fue quitado de la cola de ejecucion (el proceso sera bloqueado)", processToBlock->pid);

		blockProcess(processToBlock, true);
		freeCPUBySocket(_socket);

		(*process) = pid;
	}
}

void _blockProcess(uint32_t _socket, uint32_t* process)
{
	int32_t nbytes = 0;
	PCB_t* processToBlock = NULL;
	int32_t isDmaCall = 0;

	if((nbytes = recvPCB(_socket, &processToBlock)) <= 0)
	{
			if(nbytes == 0)
				log_error(consoleLog, "ServerThread (_blockProcess) - La CPU fue desconectada al intentar recibir un PCB");
			else
				log_error(consoleLog, "ServerThread (_blockProcess) - Error al recibir PCB de la CPU");


			FD_CLR(_socket, &master);
			closeSocketAndRemoveCPU(_socket);
	}
	else if((nbytes = receive_int(_socket, &isDmaCall)) <= 0)
	{
			if(nbytes == 0)
				log_error(consoleLog, "ServerThread (_blockProcess) - La CPU fue desconectada al intentar recibir un entero");
			else
				log_error(consoleLog, "ServerThread (_blockProcess) - Error al recibir un entero de la CPU");


			FD_CLR(_socket, &master);
			closeSocketAndRemoveCPU(_socket);
	}
	else
	{
		log_info(schedulerLog, "La CPU solicita que se bloquee el proceso %d", processToBlock->pid);

		//uint32_t operationOk = updatePCBInExecutionQueue(processToBlock); //This is useless.. It used to be that the process was updated in the executionQueue and then removed from it whan blocking that process... no more

		PCB_t* oldProcess = NULL;
		oldProcess = removeProcessFromQueueWithId(processToBlock->pid, executionQueue);

		if(oldProcess == NULL)
		{
			log_error(schedulerLog, "El proceso %d deberia estar en la cola de ejecucion, pero no se encuentra en ella, por lo que no deberia poder ser bloqueado. Este modulo sera abortado para que evalue la situacion", processToBlock->pid);
			exit(EXIT_FAILURE);
		}

		log_info(schedulerLog, "El proceso %d fue quitado de la cola de ejecucion (el sera bloqueado)", processToBlock->pid);

		blockProcess(processToBlock, true);	//send the updated process received from the CPU to the blocked queue, not the old one

		freeCPUBySocket(_socket);

		(*process) = processToBlock->pid;
	}
}

void _killProcess(uint32_t _socket)
{
	int32_t nbytes = 0;
	PCB_t* processToKill = NULL;
	int32_t processPidToKill = 0;

	if(_socket == dmaSocket)	//If the DMA sent a process error, receive the pid and kill it
	{
		if((nbytes = receive_int(_socket, &processPidToKill)) <= 0)
		{
				if(nbytes == 0)
					log_error(schedulerLog, "ServerThread (_killProcess) - El DMA fue desconectado al intentar recibir el pid de un proceso con error");
				else
					log_error(schedulerLog, "ServerThread (_killProcess) - Error al recibir el pid de un proceso con error del DMA");

				log_error(schedulerLog, "Debido a una desconexion del DMA, este proceso sera abortado...");
				exit(EXIT_FAILURE);
		}

		log_info(schedulerLog, "El DMA solicito la terminacion del proceso con id %d debido a un error", processPidToKill);


		PCB_t* oldPCB = removeProcessFromQueueWithId(processPidToKill, blockedQueue);

		if(oldPCB == NULL)
		{
			log_error(schedulerLog, "El proceso %d deberia estar en la cola de bloqueados, pero no se encuentra ella. Este modulo sera abortado para que evalue la situacion", processPidToKill);
			exit(EXIT_FAILURE);
		}

		log_info(schedulerLog, "El proceso %d fue quitado de la cola de bloqueados para terminarlo debido a un error", processPidToKill);

		killProcessWithPCB(oldPCB);
	}
	else	//If the CPU sent a process error, receive the PCB, update it in the execution queue and kill the process
	{
		if((nbytes = recvPCB(_socket, &processToKill)) <= 0)
		{
			if(nbytes == 0)
				log_error(schedulerLog, "ServerThread (_killProcess) - La CPU fue desconectada al intentar recibir un PCB\n");
			else
				log_error(schedulerLog, "ServerThread (_killProcess) - Error al recibir un PCB de la CPU\n");


			FD_CLR(_socket, &master);
			closeSocketAndRemoveCPU(_socket);
		}
		else
		{
			log_info(schedulerLog, "La CPU solicito la terminacion del proceso con id %d debido a un error durante su ejecucion", processToKill->pid);

			//uint32_t operationOk = updatePCBInExecutionQueue(processToKill); //It is useless to remove the PCB to latter send it to the finished queue (to kill that process)

			PCB_t* oldPCB = removeProcessFromQueueWithId(processToKill->pid, executionQueue);

			if(oldPCB == NULL)
			{
				log_error(schedulerLog, "El proceso %d deberia estar en la cola de ejecucion, pero no se encuentra en ella, por lo que no deberia poder ser bloqueado. Este modulo sera abortado para que evalue la situacion", processToKill->pid);
				exit(EXIT_FAILURE);
			}

			log_info(schedulerLog, "El proceso %d fue quitado de la cola de ejecucion (el sera bloqueado)", processToKill->pid);

			killProcessWithPCB(processToKill);
		}
	}

	freeCPUBySocket(_socket);
}

void processQuantumEnd(uint32_t _socket, uint32_t* process)
{
	int32_t nbytes = 0;
	PCB_t* updatedPCB = NULL;

	if((nbytes = recvPCB(_socket, &updatedPCB)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (processQuantumEnd) - La CPU fue desconectada al intentar recibir un PCB");
		else
			log_error(consoleLog, "ServerThread (processQuantumEnd) - Error al recibir un PCB de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);
	}
	else
	{
		log_info(schedulerLog, "El proceso con id %d se quedo sin quantum", updatedPCB->pid);

		//updatePCBInExecutionQueue(updatedPCB); //This is useless... better to remove the old PCB from the executionQueue and add the new one to the readyQueue

		PCB_t* oldProcess = NULL;
		oldProcess = removeProcessFromQueueWithId(updatedPCB->pid, executionQueue);

		if(oldProcess == NULL)	//the process was not in the executionQueue
		{
			log_error(schedulerLog, "El proceso %d deberia estar en la cola de ejecucion, pero no se encuentra en ella, por lo que no deberia poder ser bloqueado. Este modulo sera abortado para que evalue la situacion", updatedPCB->pid);
			exit(EXIT_FAILURE);
		}

		log_info(schedulerLog, "El proceso %d fue quitado de la cola de ejecucion (por fin de quantum)", updatedPCB->pid);

		moveProcessToReadyQueue(updatedPCB, false);

		freeCPUBySocket(_socket);

		(*process) = updatedPCB->pid;
	}
}

cpu_t* findCPUBy_socket(uint32_t _socket)
{
	//No need to lock the CPUsList here, it is already locked in the function that calls this one (freeCPUBySocket)

	cpu_t* _cpu = NULL;

	bool _cpu_has_given__socket(cpu_t* cpu)
	{
		return cpu->clientSocket == _socket;
	}
	_cpu = (cpu_t*) list_find(connectedCPUs, _cpu_has_given__socket);

	return _cpu;
}

//TODO (optional):
//		 I have to define what happens with a process being executed in a cpu that disconnected
//		 maybe rollback all its data (that is, not modifying the pcb and moving the process to the ready queue
//		 the cpu should make sure the memory rollbacks any data from that execution)
//		 THIS IS DIFFICULT TO DO AND NOT NECESSARY FOR THIS PROJECTS COMPLETION


uint32_t updatePCBInExecutionQueue(PCB_t* updatedPCB)
{
	PCB_t* oldPCB = NULL;

	bool _process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == updatedPCB->pid;
	}

	//The old pcb gets deleted from the executionQueue and the updated one gets added because I do not
	//have a proper function to replace an object in a list following a given condition

	pthread_mutex_lock(&executionQueueMutex);

	oldPCB = list_remove_by_condition(executionQueue, _process_has_given_id);

	if(oldPCB == NULL)
	{
		//ERROR; the process should not be executing!
		log_error(schedulerLog, "El PCB con id %d de la cola de ejecucion no pudo ser actualizado. Dicho proceso no deberia estar en la cola de ejecucion o, debido a un error, no llego a la misma. El modulo sera abortado para que pueda evaluar la situacion", updatedPCB->pid);
		exit(EXIT_FAILURE);
	}

	list_add(executionQueue, updatedPCB);	//Do not care about PCB arrival order in that queue

	pthread_mutex_unlock(&executionQueueMutex);

	free(oldPCB->scriptPathInFS);
	free(oldPCB);

	log_info(schedulerLog, "El PCB con id %d de la cola de ejecucion fue actualizado", updatedPCB->pid);
	return 1;
}

void checkIfFileOpen(uint32_t _socket) //Receives pid, fileName length, fileName string
{
	int32_t nbytes = 0;
	char* fileName = NULL;
	bool result = false;
	int32_t pid = 0;

	if((nbytes = receive_int(_socket, &pid)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir un pid");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir un pid de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);
	}
	else if((nbytes = receive_string(_socket, &fileName)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir un nombre de archivo");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir un nombre de archivo de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);
	}
	else
	{
		log_info(schedulerLog, "El proceso %d solicito verificar si el archivo \"%s\" fue abierto por algun proceso", pid, fileName);

		pthread_mutex_lock(&fileTableMutex);

		result = dictionary_has_key(fileTable, fileName);

		if(!result)	//Key not in the dictionary
		{
			if((nbytes = send_int_with_delay(_socket, FILE_NOT_OPEN)) < 0)
			{
				log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al indicar a la CPU que un archivo no estaba abierto");
				return;
				//TODO (Optional) - Send Error Handling
			}

			log_info(schedulerLog, "El archivo \"%s\" no fue abierto por ningun proceso. Se le informara a la CPU para que le pida al DMA que lo cargue en memoria", fileName);
		}
		else	//Key is in the dictionary
		{
			fileTableData* data = dictionary_get(fileTable, fileName);

			if(data->processFileIsAssignedTo == pid)	//If file is open for requesting process, send confirmation and file start address in memory
			{
				if((nbytes = send_int_with_delay(_socket, FILE_OPEN)) < 0)
				{
					log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al indicar a la CPU que un archivo estaba abierto");
					return;
					//TODO (Optional) - Send Error Handling
				}

				log_info(schedulerLog, "El archivo \"%s\" fue abierto por el proceso que solicito la verificacion. Es posible continuar con la ejecucion", fileName);
			}
			else if(data->processFileIsAssignedTo == 0)	//File is not claimed by any process; the requesting process will claim the file
			{
				//TODO (Optional) - Maybe change this and the "saveFileDataToFileTable" function, so the
				//					files get claimed there (when the DMA loads them in memory) and not
				//					here

				data->processFileIsAssignedTo = pid;

				if((nbytes = send_int_with_delay(_socket, FILE_OPEN)) < 0)
				{
					log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al indicar a la CPU que un archivo estaba abierto");
					return;
					//TODO (Optional) - Send Error Handling
				}

				log_info(schedulerLog, "El archivo \"%s\" se encuentra en la tabla de archivos pero no esta asignado a ningun proceso. Se le asignara al proceso %d", fileName, pid);
			}
			else	//If file not open by requesting process, send confirmation so the CPU asks to block the process; then add the process to the file waiting list
			{
				if((nbytes = send_int_with_delay(_socket, FILE_OPENED_BY_ANOTHER_PROCESS)) < 0)
				{
					log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al indicar a la CPU que un archivo estaba abierto por otro proceso");
					return;
					//TODO (Optional) - Send Error Handling
				}

				log_info(schedulerLog, "El archivo \"%s\" fue abierto por el proceso %d. El proceso que lo solicitaba (con id %d) sera agregado a la lista de espera del archivo", data->processFileIsAssignedTo, pid);

				t_list* fileWaitingList = data->processesWaitingForFile;

				list_add(fileWaitingList, pid);
			}
		}
	}

	pthread_mutex_unlock(&fileTableMutex);

	/*Receive file name and check if the process has that file open or if that file has been opened.
	  If the file is not in the file table (after close/flush o if the process is killed/terminated,
	  files are closed and removed from memory and file table), tell the cpu so it issues a process block request

	  If the file is in the file table, it may be there because there were processes waiting for it, or
	  because another process claimed it.
	  In that case, check if the file is claimed by any process. If so, block the requesting process
	  and put it in the file's wait list.
	  If the file is claimed by a process with id 0, then it is claimed by no processes, so the
	  requesting process will claim it.


	  Actually, I wanted to make it so the processes did not claim the files if they were in the table
	  but free. I wanted to make it so the files got claimed when the DMA told this module to save the
	  file data in the fileTable.
	  But, because I had little time to finish this module and think what to do if a process claimed a
	  file before the one that called this routine could, I opted to allow the process that called this
	  routine to claim the file now.
	  */
}

bool saveFileDataToFileTable(char* filePath, uint32_t pid) //Receives pid, fileName length, fileName string
{
	bool result = false;
	fileTableData* data = NULL;


	pthread_mutex_lock(&fileTableMutex);
	pthread_mutex_lock(&fileTableKeysMutex);

	result = dictionary_has_key(fileTable, filePath);

	if(result)	//The key is in the fileTable, maybe because there are processes waiting for it
	{
		//No need to update anything
		data = dictionary_get(fileTable, filePath);

		if((data->processFileIsAssignedTo != 0) && (data->processFileIsAssignedTo != pid))
		{
			//ERROR - A process is asking for a file that was taken by another process..
			//		  this should not happen; the requesting process will be terminated

			log_error(schedulerLog, "ERROR - El archivo \"%s\" fue reservado por el proceso %d, y no podra ser asignado al proceso %d (el cual el DMA solicito desbloquear)", data->processFileIsAssignedTo, pid);
			killProcess(pid);
			return true;

			//TODO - Remove terminated process' script from the memory (for that, I need to send that request to a CPU)
		}
		else if(data->processFileIsAssignedTo == 0)
		{
			//ERROR - The file did not get claimed by the process which told
			//		  the DMA to load it in memory


			log_error(schedulerLog, "ERROR - El archivo \"%s\" no fue reservado por el proceso %d antes de solicitar al DMA que lo cargue en memoria", pid);
			killProcess(pid);
			return true;

			//TODO - Remove terminated process' script from the memory (for that, I need to send that request to a CPU)
		}
	}
	else //Key is not in the fileTable; add the key and fill the key data with the values obtained from the DMA
	{
		fileTableData* data = calloc(1, sizeof(fileTableData));

		data->processFileIsAssignedTo = pid;
		data->processesWaitingForFile = list_create();

		dictionary_put(fileTable, filePath, data);
		list_add(fileTableKeys, filePath);

		log_info(schedulerLog, "Se agrego una entrada a la tabla de archivos para el archivo \"%s\", el cual fue asignado al proceso %d", filePath, pid);
	}

	pthread_mutex_unlock(&fileTableMutex);
	pthread_mutex_unlock(&fileTableKeysMutex);

	return false;
}

void closeFile(uint32_t _socket)
{
	int32_t nbytes = 0;
	int32_t pid = 0;
	int32_t processToUnblock = 0;
	fileTableData* data = NULL;
	fileTableData* dataToRemove = NULL;
	t_list* processWaitList = NULL;
	char* fileName = NULL;
	bool result = false;


	if((nbytes = receive_int(_socket, &pid)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir un pid");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir un pid de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);
	}
	else if((nbytes = receive_string(_socket, &fileName)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir un nombre de archivo");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir un nombre de archivo de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);
	}
	else
	{
		pthread_mutex_lock(&fileTableMutex);
		pthread_mutex_lock(&fileTableKeysMutex);

		result = dictionary_has_key(fileTable, fileName);

		if(result)	 //The fileTable has the key, it can be removed
		{
			data = dictionary_get(fileTable, fileName);
			processWaitList = data->processesWaitingForFile;
			uint32_t processesWaitingForFile = list_size(processWaitList);

			//Each time a process requests a file to be closed it gets flushed from the memory, and removed
			//from the fileTable and the memory. If 1 or more processes are blocked waiting for the file,
			//all of them are unblocked...

			log_info(schedulerLog, "El archivo \"%s\" sera quitado de la tabla de archivos, ya que el proceso %d solicito cerrarlo", fileName, pid);

			if(processesWaitingForFile == 0) //No processes waiting for the file; the key can be removed
			{
				dataToRemove = dictionary_remove(fileTable, fileName);
				removeKeyFromList(fileTableKeys, fileName);

				list_destroy(dataToRemove->processesWaitingForFile);
				free(dataToRemove);

				log_info(schedulerLog, "No existen procesos esperando a que el archivo \"%s\" sea liberado", fileName);
			}
			else	//Unblock the waiting processes and remove the key; a process should not request to close a file if it was not assigned to it, so no need to check if the file is assignet to that process
			{
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

				dataToRemove = dictionary_remove(fileTable, fileName);
				list_destroy(dataToRemove->processesWaitingForFile);
				free(dataToRemove);

				log_info(schedulerLog, "Los siguientes procesos esperaban por la liberacion del archivo \"%s\" y fueron desbloqueados: %s", fileName, processesWaitingForFile);

				free(procWaitingForFileString);
			}
		}
		else	//The fileTable does not have the key
		{
			//TODO (Optional) - Handle the error - The fileTable does not contain the requested file
			//										This should never happen

			log_error(schedulerLog, "Se intento cerrar el archivo \"%s\", pero el mismo no esta presente en la tabla de archivos", fileName);
		}

		pthread_mutex_unlock(&fileTableMutex);
		pthread_mutex_unlock(&fileTableKeysMutex);
	}
}

void unlockProcess(uint32_t _socket)
{
	int32_t processId = 0;
	int32_t nbytes = 0;
	char* filePath = NULL;
	int32_t addFileToFileTable = 0;
	bool processWasKilled = false;

	if((nbytes = receive_int(_socket, &processId)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (unlockProcess) - El DMA fue desconectado al intentar recibir un pid");
		else
			log_error(consoleLog, "ServerThread (unlockProcess) - Error al recibir un pid del DMA");


		FD_CLR(_socket, &master);
		close(_socket);
	}
	else if((nbytes = receive_string(_socket, &filePath)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (unlockProcess) - El DMA fue desconectado al intentar recibir el path del archivo abierto");
		else
			log_error(consoleLog, "ServerThread (unlockProcess) - Error al recibir el path del archivo abierto del DMA");

		FD_CLR(_socket, &master);
		close(_socket);
	}
	else if((nbytes = receive_int(_socket, &addFileToFileTable)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (unlockProcess) - El DMA fue desconectado al intentar recibir un mensaje indicando si el archivo abierto es un script");
		else
			log_error(consoleLog, "ServerThread (unlockProcess) - Error al recibir un mensaje indicando si el archivo abierto es un script del DMA");

		FD_CLR(_socket, &master);
		close(_socket);
	}
	else
	{
		log_info(schedulerLog, "El DMA solicita desbloquear el proceso %d", processId);

		//TODO (optional) - Maybe, after modifying the below and the "checkIfFileOpen" functions,
		//					I could add a "bool" pointer to be modified in the below function, and let
		//					me decide whether to unblock the process or not (if the file got claimed by
		//					another process, the current one should not be unblocked)

		if(addFileToFileTable)
			processWasKilled = saveFileDataToFileTable(filePath, processId);

		if(!processWasKilled)
			unblockProcess(processId, true);
	}
}

void signalResource(uint32_t _socket)
{
	int32_t nbytes = 0;
	char* semaphoreName = NULL;
	bool result = 0;
	semaphoreData* data = NULL;

	if((nbytes = receive_string(_socket, &semaphoreName)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (waitResource) - La CPU fue desconectada al intentar recibir un nombre de semaforo");
		else
			log_error(consoleLog, "ServerThread (waitResource) - Error al recibir un nombre de semaforo de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);
	}
	else
	{
		pthread_mutex_lock(&semaphoreListMutex);

		result = dictionary_has_key(semaphoreList, semaphoreName);

		if(result)
		{
			data = dictionary_get(semaphoreList, semaphoreName);
			(data->semaphoreValue)++;

			if((nbytes = send_int_with_delay(_socket, SIGNAL_OK)) < 0)
			{
				log_error(consoleLog, "ServerThread (signalResource) - Error al indicar a la CPU que un signal fue exitoso");
				return;
				//TODO (Optional) - Send Error Handling
			}

			log_info(schedulerLog, "No existia ningun semaforo con el nombre \"%s\", por lo que este fue creado e inicializado con valor 1", semaphoreName);
		}
		else	//The key is not in the dictionary, create it with value 1
		{
			data = calloc(1, sizeof(semaphoreData));
			data->semaphoreValue = 1;
			data->waitingProcesses = list_create();

			if((nbytes = send_int_with_delay(_socket, SIGNAL_OK)) < 0)
			{
				log_error(consoleLog, "ServerThread (signalResource) - Error al indicar a la CPU que un signal fue exitoso");
				return;
				//TODO (Optional) - Send Error Handling
			}

			log_info(schedulerLog, "El valor del semaforo \"%s\" fue incrementado", semaphoreName);
		}

		pthread_mutex_unlock(&semaphoreListMutex);
	}
}

void waitResource(uint32_t _socket)
{
	int32_t pid = 0;
	int32_t nbytes = 0;
	char* semaphoreName = NULL;
	bool dictionaryHasKey = 0;
	semaphoreData* data = NULL;
	t_list* processWaitList = NULL;

	if((nbytes = receive_int(_socket, &pid)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (waitResource) - La CPU fue desconectada al intentar recibir un pid");
		else
			log_error(consoleLog, "ServerThread (waitResource) - Error al recibir un pid de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);
	}
	else if((nbytes = receive_string(_socket, &semaphoreName)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (waitResource) - La CPU fue desconectada al intentar recibir un nombre de semaforo");
		else
			log_error(consoleLog, "ServerThread (waitResource) - Error al recibir un nombre de semaforo de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);
	}
	else
	{
		pthread_mutex_lock(&semaphoreListMutex);

		dictionaryHasKey = dictionary_has_key(semaphoreList, semaphoreName);

		if(dictionaryHasKey)
		{
			data = dictionary_get(semaphoreList, semaphoreName);
			processWaitList = data->waitingProcesses;

			if(data->semaphoreValue > 0)
			{
				(data->semaphoreValue)--;
				list_add(data->processesUsingTheSemaphore, pid);

				if((nbytes = send_int_with_delay(_socket, WAIT_OK)) < 0)
				{
					log_error(consoleLog, "ServerThread (waitResource) - Error al indicar a la CPU que un wait fue exitoso");
					return;
					//TODO (Optional) - Send Error Handling
				}

				log_info(schedulerLog, "Se reservo una instancia del semaforo \"%s\" para el proceso %d", semaphoreName, pid);
			}
			else	//Add the process to the semaphore wait list, and tell the CPU the wait could not be done, so it requests a process block
			{
				list_add(processWaitList, pid);

				if((nbytes = send_int_with_delay(_socket, WAIT_ERROR)) < 0)
				{
					log_error(consoleLog, "ServerThread (waitResource) - Error al indicar a la CPU que un wait no pudo realizarse");
					return;
					//TODO (Optional) - Send Error Handling
				}

				log_info(schedulerLog, "El proceso %d solicito una instancia del semaforo \"%s\", pero este no tiene instancias disponibles. El proceso sera agregado a la lista de espera del semaforo", pid, semaphoreName);
			}
		}
		else	//The key is not in the dictionary, create it with value 1 and let the process wait on it
		{
			data = calloc(1, sizeof(semaphoreData));
			data->semaphoreValue = 0;	//Created with value 0 (already taken by the requesting process)
			data->waitingProcesses = list_create();
			data->processesUsingTheSemaphore = list_create();

			dictionary_put(semaphoreList, semaphoreName, data);

			pthread_mutex_lock(&semaphoreListKeysMutex);

			list_add(semaphoreListKeys, semaphoreName);

			pthread_mutex_unlock(&semaphoreListKeysMutex);

			if((nbytes = send_int_with_delay(_socket, WAIT_OK)) < 0)
			{
				log_error(consoleLog, "ServerThread (waitResource) - Error al indicar a la CPU que un wait fue exitoso");
				return;
				//TODO (Optional) - Send Error Handling
			}

			log_info(schedulerLog, "El semaforo \"%s\" no existia, por lo que fue creado con 1 instancia y asignado al proceso %d", semaphoreName, pid);
		}

		pthread_mutex_unlock(&semaphoreListMutex);
	}
}

void freeCPUBySocket(uint32_t _socket)
{
	//Need to lock the CPUsList here, so no there is no risk of the current CPU getting modified when this function is running

	pthread_mutex_lock(&cpuListMutex);

	cpu_t* cpuToFree =  NULL;
	cpuToFree = findCPUBy_socket(_socket);

	if(cpuToFree == NULL)
	{
		log_error(schedulerLog, "ERROR - Se indico liberar una CPU por su socket, pero esta no fue encontrada en la lista de CPUs conectadas. El modulo sera abortado aprta que evalue la situacion");
		exit(EXIT_FAILURE);
	}

	cpuToFree->currentProcess = 0;
	cpuToFree->isFree = true;

	checkAndInitializeProcesses(cpuToFree); //check if there are any processes left to inialize, and do it with the new CPU

/*	if((cpuToFree->isFree) && (stsWantsToExecute))	//If after trying to initialize a process, the CPU is still free, check if the STS wants to run and wake it up
	{
		log_info(schedulerLog, "El STS queria ejecutarse y se conecto una CPU. Se le permitira ejecutar al STS");

		uint32_t semaphoreValue;

		sem_getvalue(&shortTermScheduler, &semaphoreValue);

		if(semaphoreValue == 0)		//If semaphore value < 0, the STS may already been executing
			sem_post(&shortTermScheduler);
	}
*/ //Removed to avoid problems with semaphores


	//If after trying to initialize processes with the current CPU, that CPU remains free
	//(which means no process was send to be initialized to that CPU), then check if there are processes ready to be
	//executed, and if the STS was not posted as many times as there are ready processes; in that case, post the STS so it executes
	//the remaining processes with the available CPU
	int32_t timesTheSTSWasCalled;
	sem_getvalue(&shortTermScheduler, &timesTheSTSWasCalled);

	//if((cpuToFree->isFree) && (processesReadyToExecute > 0) && (processesReadyToExecute > timesTheSTSWasCalled))
	if((cpuToFree->isFree) && (processesReadyToExecute > timesTheSTSWasCalled))
	{
		sem_post(&shortTermScheduler);
	}


	pthread_mutex_unlock(&cpuListMutex);
}

void handleCpuConnection(uint32_t _socket)
{
	int32_t nbytes = 0;

	log_info(consoleLog, "Nueva conexion de CPU\n");

	if(config.schedulingAlgorithmCode == Custom)
	{
		if((nbytes = send_int_with_delay(_socket, USE_CUSTOM_ALGORITHM)) < 0)
		{
			log_error(consoleLog, "ServerThread (handleCpuConnection) - Error al indicar a la CPU que el primer mensaje de handshake fue recibido");
			return;
			//TODO (Optional) - Send Error Handling
		}
	}
	else
	{
		if((nbytes = send_int_with_delay(_socket, USE_NORMAL_ALGORITHM)) < 0)
		{
			log_error(consoleLog, "ServerThread (handleCpuConnection) - Error al indicar a la CPU que el primer mensaje de handshake fue recibido");
			return;
			//TODO (Optional) - Send Error Handling
		}
	}



	cpu_t* newCPU = calloc(1, sizeof(cpu_t));
	newCPU->cpuId = ++totalConnectedCpus;
	newCPU->currentProcess = 0;
	newCPU->isFree = true;
	newCPU->clientSocket = _socket;

	pthread_mutex_lock(&cpuListMutex);

	list_add(connectedCPUs, newCPU);
	checkAndInitializeProcesses(newCPU); //check if there are any processes left to inialize, and do it with the new CPU

/*	if((newCPU->isFree) && (stsWantsToExecute))	//If after trying to initialize a process, the CPU is still free, check if the STS wants to run and wake it up
	{
		log_info(schedulerLog, "El STS queria ejecutarse y se conecto una CPU. Se le permitira ejecutar al STS");

		int32_t semaphoreValue;

		sem_getvalue(&shortTermScheduler, &semaphoreValue);

		if(semaphoreValue == 0)		//If semaphore value < 0, the STS may already been executing
			sem_post(&shortTermScheduler);
	}
*/	//Remove to avoid problems with semaphores


	//If after trying to initialize processes with the current CPU, that CPU remains free
	//(which means no process was send to be initialized to that CPU), then check if there are processes ready to be
	//executed, and if the STS was not posted as many times as there are ready processes; in that case, post the STS so it executes
	//the remaining processes with the available CPU
	int32_t timesTheSTSWasCalled;
	sem_getvalue(&shortTermScheduler, &timesTheSTSWasCalled);

	//if((newCPU->isFree) && (processesReadyToExecute > 0) && (processesReadyToExecute > timesTheSTSWasCalled))
	if((newCPU->isFree) && (processesReadyToExecute > timesTheSTSWasCalled))
	{
		sem_post(&shortTermScheduler);
	}


	pthread_mutex_unlock(&cpuListMutex);
}

void terminateProcess(uint32_t _socket)
{
	int32_t nbytes = 0;
	PCB_t* updatedPCB = NULL;

	if((nbytes = recvPCB(_socket, &updatedPCB)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (terminateProcess) - La CPU fue desconectada al intentar recibir un PCB");
		else
			log_error(consoleLog, "ServerThread (terminateProcess) - Error al recibir un PCB de la CPU");


		FD_CLR(_socket, &master);
		closeSocketAndRemoveCPU(_socket);
	}

	//updatePCBInExecutionQueue(updatedPCB); //Useless to update the process in the executionQueue to latter remove that process from that queue

	PCB_t* oldPCB = NULL;
	oldPCB = removeProcessFromQueueWithId(updatedPCB->pid, executionQueue);

	if(oldPCB == NULL)
	{
		log_error(schedulerLog, "El proceso %d deberia estar en la cola de ejecucion, pero no se encuentra en ella, por lo que no deberia poder ser bloqueado. Este modulo sera abortado para que evalue la situacion", updatedPCB->pid);
		exit(EXIT_FAILURE);
	}

	log_info(schedulerLog, "El proceso %d fue quitado de la cola de ejecucion (el proceso se esta finalizando)", updatedPCB->pid);
	terminateExecutingProcess(updatedPCB);

	freeCPUBySocket(_socket);
}

void handleConfigFileChanged()
{
	uint32_t configFilePathSize = 27;
	size_t bufferSize = sizeof(struct inotify_event) + configFilePathSize + 1;	//Reserves additional space for the path variable inside the struct
	struct inotify_event* event = malloc(bufferSize);

	read(configFileInotifyFD, event, bufferSize);

	if(event->mask == IN_MODIFY)	//If the event was because of a modification
	{
		pthread_mutex_lock(&configFileMutex);

		int32_t result = getConfigs();

		pthread_mutex_unlock(&configFileMutex);

		if(result < 0)
		{
			if(result == MALLOC_ERROR)
			{
				log_error(schedulerLog, "Se detecto una modificacion en el archivo de configuracion, pero no pudieron obtenerse los cambios por un error de malloc...");
			}
			else if (result == CONFIG_PROPERTY_NOT_FOUND)
			{
				log_error(schedulerLog, "Se detecto una modificacion en el archivo de configuracion, pero no pudieron obtenerse los cambios debido a que faltan una o mas propiedades...");
			}

			log_error(consoleLog, "Se detecto una modificacion en el archivo de configuracion, pero ocurrio un error al obtener las nuevas configuraciones");
		}

		//TODO - Explanation about the Scheduling algorithm used and the PCB's "instructionsUntilIoOrEnd" variable

		//There is no need to tell each one of the connected CPUs of the scheduling algorithm changed, because the PCB variable that counts how many
		//instructions are left until the next IO instruction gets changed based on the algorithm that is used.

		//If the custom algorithm is used, that variable will have a value other than 0, and it will be updated each time a process is sent to a CPU.
		//If a process was sent to a CPU and the algorithm changed from custom to RR/VRR, although that variable gets updated, it is not used by the VRR/RR algorithms

		//If the VRR/RR algorithm is used, that variable will have a value of 0, and will not be updated when a process exits a CPU.
		//If a process was sent to a CPU and the algorithm changed from VRR/RR to custom, when the variable needs to be used to schedule processes, if
		//that process is in the READY queue, that variable from its PCB will be updated and them will be updated when it exits a CPU
	}
	else
	{
		//TODO (Optional) - The mask should not have another value than "IN_MODIFY" -> ERROR
	}

	free(event);
}
