#define LIM_INF_TAREAS_CPU 6
#define LIM_SUP_TAREAS_CPU 25
#define LIM_INF_TAREAS_DMA 26
#define LIM_SUP_TAREAS_DMA 30

#include "Scheduler.h"

void server()
{
	fd_set read_fds; //Temporal fd set for the 'select()' function
	struct sockaddr_in remoteaddr; //Client address
	uint32_t fdmax; //Number of the max fd I got
	uint32_t newfd; //Accepted connection socket
	int32_t command; //Client command
	uint32_t nbytes;
	uint32_t addrlen;
	FD_ZERO(&master); //Delete master & read sets
	FD_ZERO(&read_fds);

	//Get listener socket
	pthread_mutex_lock(&configFileMutex);

	uint32_t servidor = build_server(config.port, consoleLog);

	pthread_mutex_unlock(&configFileMutex);

	//Adding listener socket to the master set
	FD_SET(servidor, &master);
	FD_SET(configFileInotifyFD, &master);

	if(servidor >= configFileInotifyFD)	//Check if the max fd number is assigned to the Inotify watch or to the server socket
		fdmax = servidor;
	else
		fdmax = configFileInotifyFD;

	//Main loop
	while (!terminateModule)
	{
		read_fds = master; //Copy the master set
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			log_error(consoleLog, "Error en select\n");
			exit(EXIT_FAILURE);
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
						log_error(consoleLog, "Error en accept\n");
					else
					{
						FD_SET(newfd, &master); //Add the socket to the master set
						if (newfd > fdmax) //Update the max fd
							fdmax = newfd;
					}
				}
				else if(i == configFileInotifyFD)
				{
					uint32_t configFilePathSize = 27;
					size_t bufferSize = sizeof(struct inotify_event) + configFilePathSize + 1;	//Reserves additional space for the path variable inside the struct
					struct inotify_event* event = malloc(bufferSize);

					read(configFileInotifyFD, event, bufferSize);

					if(event->mask == IN_MODIFY)	//If the event was because of a modification
					{
						int32_t result = getConfigs();

						if(result < 0)
						{
							if(result == MALLOC_ERROR)
							{
								log_error(schedulerLog, "Se detecto una modificacion en el archivo de configuracion, pero no pudieron obtenerse los cambios por un error de malloc...\n");
							}
							else if (result == CONFIG_PROPERTY_NOT_FOUND)
							{
								log_error(schedulerLog, "Se detecto una modificacion en el archivo de configuracion, pero no pudieron obtenerse los cambios debido a que faltan una o mas propiedades...\n");
							}

							log_error(consoleLog, "Se detecto una modificacion en el archivo de configuracion, pero ocurrio un error al obtener las nuevas configuraciones");
						}
					}
					else
					{
						//TODO (Optional) - The mask should not have another value than "IN_MODIFY" -> ERROR
					}

					free(event);
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
								log_info(consoleLog, "DMA desconectado\n");
							else
								log_info(consoleLog, "CPU desconectada\n");
						}
						else
							log_error(consoleLog, "recv (comando)\n");

						close(i);
						FD_CLR(i, &master); //Remove socket from master set
					}
					else
					{
						//Received data from a client
						moduleHandler(command, i);
					}
				}
			} // if (FD_ISSET(i, &read_fds))
		}
	} // while (true)
}

void moduleHandler(uint32_t command, uint32_t socket)
{
	if(command == NEW_DMA_CONNECTION)
	{
						  
		log_info(consoleLog, "Nueva conexion desde el DMA\n");
		dmaSocket = socket;
	}
	else if(command == NEW_CPU_CONNECTION)
		handleCpuConnection(socket);
	else if((command >= LIM_INF_TAREAS_CPU) && (command <= LIM_SUP_TAREAS_CPU)) //The CPU task codes range from 3 to 12
		cpuTaskHandler(command, socket);

	else if((command >= LIM_INF_TAREAS_DMA) && (command <= LIM_SUP_TAREAS_DMA)) //The DMA task codes range from 13 to 18
		dmaTaskHandler(command, socket);

	else
		log_error(consoleLog, "ServerThread - La tarea recibida, con codigo %d, es incorrecta\n", command);
}

void dmaTaskHandler(uint32_t task, uint32_t socket)
{
	switch(task)
	{
		case UNLOCK_PROCESS:
			unlockProcess(socket);
		break;

		default:
			log_error(consoleLog, "ServerThread - Se recibio una tarea incorrecta del DMA (codigo = %d)\n", task);
		break;
	}
}

void cpuTaskHandler(uint32_t task, uint32_t socket)
{
	switch(task)
	{
		case COMPLETED_INSTRUCTION:
			pthread_mutex_lock(&metricsGlobalvariablesMutex);
			executedInstructions++;
			pthread_mutex_unlock(&metricsGlobalvariablesMutex);
		break;

		case BLOCK_PROCESS_INIT:
			blockProcessInit(socket);
		break;

		case BLOCK_PROCESS:
			_blockProcess(socket);
		break;

		case PROCESS_ERROR:
			_killProcess(socket);
		break;

		case QUANTUM_END:
			processQuantumEnd(socket);
		break;

		case CHECK_IF_FILE_OPEN:		//Checks if the given file is open for the issuing process, and tries to open it
			checkIfFileOpen(socket);
		break;

		case CLOSE_FILE:
			closeFile(socket);
		break;

		case WAIT_RESOURCE:		//Tries to wait on a semaphore and tells the CPU if it was possible
			waitResource(socket);
		break;

		case SIGNAL_RESOURCE:		//Tries to signal a semaphore and tells the CPU if it was possible
			signalResource(socket);
		break;

		default:
			log_error(consoleLog, "ServerThread - Se recibio una tarea incorrecta de la CPU (codigo = %d)\n", task);
		break;

	}

	//If any of these tasks were received, the CPU became free, so it can be assigned to another process
	if((task == BLOCK_PROCESS_INIT) || (task == BLOCK_PROCESS_INIT) ||
			(task == BLOCK_PROCESS_INIT) || (task == BLOCK_PROCESS_INIT))
	{
		cpu_t* currentCPU = findCPUBySocket(socket);

		checkAndInitializeProcesses(currentCPU); //check if there are any processes left to initialize, and do it with the free CPU
	}
}

void blockProcessInit(uint32_t socket)
{
	int32_t pid;
	int32_t nbytes;	//This cannot be unsigned; check the recv below

	if((nbytes = receive_int(socket, &pid)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (blockProcessInit) - La CPU fue desconectada al intentar recibir un pid\n");
		else
			log_error(consoleLog, "ServerThread (blockProcessInit) - Error al recibir un pid de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);

		//TODO (optional):
		//		 If a CPU disconnects while executing a process, it should tell all the other
		//		 modules and they should roll back all the changes that were made for that process
		//		 that was executing in the dead CPU.
		//		 Something like that should happen with all the other modules...
		//		 To achieve this, a module should have a signal catch (to avoid it closing without notice)
		//		 and when a module knows it will die (or another module died), it should tell all
		//		 the other modules connected to it...
	}
	else
	{
		blockProcess(pid, true);
		freeCPUBySocket(socket);
	}
}

void _blockProcess(uint32_t socket)
{
	int32_t nbytes;
	PCB_t* processToBlock = NULL;
	int32_t isDmaCall;

	if((nbytes = recvPCB(processToBlock, socket)) <= 0)
	{
			if(nbytes == 0)
				log_error(consoleLog, "ServerThread (_blockProcess) - La CPU fue desconectada al intentar recibir PCB\n");
			else
				log_error(consoleLog, "ServerThread (_blockProcess) - Error al recibir PCB de la CPU\n");

			closeSocketAndRemoveCPU(socket);
			FD_CLR(socket, &master);
	}
	else if((nbytes = receive_int(socket, &isDmaCall)) <= 0)
	{
			if(nbytes == 0)
				log_error(consoleLog, "ServerThread (_blockProcess) - La CPU fue desconectada al intentar recibir un entero \n");
			else
				log_error(consoleLog, "ServerThread (_blockProcess) - Error al recibir un entero de la CPU\n");

			closeSocketAndRemoveCPU(socket);
			FD_CLR(socket, &master);
	}
	else
	{
		uint32_t operationOk = updatePCBInExecutionQueue(processToBlock);

		if(!operationOk)
		{
			//TODO (optional) - Handle the error (the process was not in the execution queue)
		}

		blockProcess(processToBlock->pid, isDmaCall); //Could send the PCB to that function, but it is also used by the
													  //function that blocks a process being initialized (which has no
													  //new PCB to send to the blockProcess function)
	}
}

void _killProcess(uint32_t socket)
{
	int32_t nbytes;
	PCB_t* processToKill = NULL;

	if((nbytes = recvPCB(processToKill, socket)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (_killProcess) - La CPU fue desconectada al intentar recibir un PCB\n");
		else
			log_error(consoleLog, "ServerThread (_killProcess) - Error al recibir un PCB de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else
	{
		uint32_t operationOk = updatePCBInExecutionQueue(processToKill);

		if(!operationOk)
		{
			//TODO (optional) - Handle the error (the process was not in the execution queue)
		}

		killProcess(processToKill->pid); //Could send the PCB to that function, but it is also used by the
										 //console (which only sends the process id)
	}
}

void processQuantumEnd(uint32_t socket)
{
	int32_t nbytes;
	PCB_t* updatedPCB;

	if((nbytes = recvPCB(updatedPCB, socket)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (quantumEnd) - La CPU fue desconectada al intentar recibir un PCB\n");
		else
			log_error(consoleLog, "ServerThread (quantumEnd) - Error al recibir un PCB de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else
	{
		bool _process_has_given_id(PCB_t* pcb)
		{
			return pcb->pid == updatedPCB->pid;
		}

		PCB_t* processRemoved = list_remove_by_condition(executionQueue, _process_has_given_id);

		if(processRemoved == NULL)
		{
			//TODO (Optional) - Handle the error - The process was not in the executionQueue
		}

		log_info(schedulerLog, "El proceso con id %d se quedo sin quantum\n", updatedPCB->pid);

		moveProcessToReadyQueue(updatedPCB, false);
	}
}

cpu_t* findCPUBySocket(uint32_t socket)
{
	bool _cpu_has_given_socket(cpu_t* cpu)
	{
		return cpu->clientSocket == socket;
	}

	return (cpu_t*) list_find(connectedCPUs, _cpu_has_given_socket);
}

//TODO (optional):
//		 I have to define what happens with a process being executed in a cpu that disconnected
//		 maybe rollback all its data (that is, not modifying the pcb and moving the process to the ready queue
//		 the cpu should make sure the memory rollbacks any data from that execution)
//		 THIS IS DIFFICULT TO DO AND NOT NECESSARY FOR THIS PROJECTS COMPLETION


void checkAndInitializeProcesses(cpu_t* freeCpu)
{
	//Check if there are any uninitialized processes in the readyQueue. If there are any, remove the
	//first one from that queue and initialize it with the given CPU

	pthread_mutex_lock(&readyQueueMutex);

	bool _process_is_uninitialized(PCB_t* pcb)
	{
		return !(pcb->wasInitialized);
	}

	//The new list should have all the uninitialized processes ordered by which time they arrived
	//to the readyQueue
	t_list* uninitializedProcesses = list_filter(readyQueue, _process_is_uninitialized);

	if(list_size(uninitializedProcesses) != 0)
	{
		PCB_t* processToInitialize = list_get(uninitializedProcesses, 0);
		processToInitialize->wasInitialized = false;

		bool _process_has_given_id(PCB_t* pcb)
		{
			return pcb->pid == processToInitialize->pid;
		}

		list_remove_by_condition(readyQueue, _process_has_given_id);

		executeProcess(processToInitialize, freeCpu);
	}
}

uint32_t updatePCBInExecutionQueue(PCB_t* updatedPCB)
{
	bool _process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == updatedPCB->pid;
	}

	//The old pcb gets deleted from the executionQueue and the updated one gets added because I do not
	//have a proper function to replace an object in a list following a given condition

	pthread_mutex_lock(&executionQueueMutex);

	PCB_t* oldPCB = list_remove_by_condition(executionQueue, _process_has_given_id);

	pthread_mutex_unlock(&executionQueueMutex);

	if(oldPCB == NULL)
	{
		//ERROR; the process should not be executing!
		log_error(schedulerLog, "El PCB con id %d de la cola de ejecucion no pudo ser actualizado\n", updatedPCB->pid);
		return 0;
	}

	free(oldPCB->scriptName);
	free(oldPCB);

	pthread_mutex_lock(&executionQueueMutex);

	list_add(executionQueue, updatedPCB);	//Do not care about PCB arrival order in that queue

	pthread_mutex_unlock(&executionQueueMutex);

	log_info(schedulerLog, "El PCB con id %d de la cola de ejecucion fue actualizado\n", updatedPCB->pid);
	return 1;
}

void checkIfFileOpen(uint32_t socket) //Receives pid, fileName length, fileName string
{
	int32_t nbytes;
	char* fileName;
	bool result;
	uint32_t pid;

	pthread_mutex_lock(&fileTableMutex);

	if((nbytes = receive_int(socket, &pid)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir un pid\n");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir un pid de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else if((nbytes = receive_string(socket, &fileName)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir un nombre de archivo\n");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir un nombre de archivo de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else
	{
		result = dictionary_has_key(fileTable, fileName);

		if(!result)	//Key not in the dictionary
		{
			if((nbytes = send_int_with_delay(socket, FILE_NOT_OPEN)) < 0)
			{
				log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al indicar a la CPU que un archivo no estaba abierto\n");
				return;
				//TODO (Optional) - Send Error Handling
			}
		}
		else	//Key is in the dictionary
		{
			fileTableData* data = dictionary_get(fileTable, fileName);

			if(data->processFileIsAssignedTo == pid)	//If file is open for requesting process, send confirmation and file start address in memory
			{
				if((nbytes = send_int_with_delay(socket, FILE_OPEN)) < 0)
				{
					log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al indicar a la CPU que un archivo estaba abierto\n");
					return;
					//TODO (Optional) - Send Error Handling
				}

				if((nbytes = send_int_with_delay(socket, data->memoryStartAddress)) < 0)
				{
					log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al indicar a la CPU que un archivo estaba abierto\n");
					return;
					//TODO (Optional) - Send Error Handling
				}
			}
			else if(data->processFileIsAssignedTo == 0)	//File is not claimed by any process; the requesting process will claim the file
			{
				//TODO (Optional) - Maybe change this and the "saveFileDataToFileTable" function, so the
				//					files get claimed there (when the DMA loads them in memory) and not
				//					here

				data->processFileIsAssignedTo = pid;
			}
			else	//If file not open by requesting process, send confirmation so the CPU asks to block the process; then add the process to the file waiting list
			{
				if((nbytes = send_int_with_delay(socket, FILE_OPENED_BY_ANOTHER_PROCESS)) < 0)
				{
					log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al indicar a la CPU que un archivo estaba abierto por otro proceso\n");
					return;
					//TODO (Optional) - Send Error Handling
				}

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

void saveFileDataToFileTable(uint32_t socket, uint32_t pid) //Receives pid, fileName length, fileName string, file start address in memory
{
	int32_t nbytes;
	char* fileName;
	int32_t memoryStartAddress;
	bool result;
	fileTableData* data;

	pthread_mutex_lock(&fileTableMutex);

	if((nbytes = receive_string(socket, &fileName)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (saveFileDataToFileTable) - El DMA fue desconectado al intentar recibir un nombre de archivo\n");
		else
			log_error(consoleLog, "ServerThread (saveFileDataToFileTable) - Error al recibir un nombre de archivo del DMA\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else if((nbytes = receive_int(socket, &memoryStartAddress)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (saveFileDataToFileTable) - El DMA fue desconectado al intentar recibir direccion de memoria\n");
		else
			log_error(consoleLog, "ServerThread (saveFileDataToFileTable) - Error al recibir direccion de memoria del DMA\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else
	{
		result = dictionary_has_key(fileTable, fileName);

		if(result)	//The key is in the fileTable, maybe because there are processes waiting for it
		{
			//No need to update anything, but I will update the memory address, just in case
			data = dictionary_get(fileTable, fileName);
			data->memoryStartAddress = memoryStartAddress;

			if((data->processFileIsAssignedTo != 0) && (data->processFileIsAssignedTo != pid))
			{
				//TODO - ERROR - Another process claimed the file before this one; should never happen
				//				 because processes claim the files before requesting the DMA to
				//				 load them in memory
			}
			else if(data->processFileIsAssignedTo == 0)
			{
				//TODO (optional) - ERROR - The file did not get claimed by the process which told
				//							the DMA to load it in memory

				////TODO (Optional) - Maybe change this whole function, so the files get claimed here
				//					  exactly when this condition is met. In that scenario, the above
				//					  condition could be possible and should be handled properly
			}
		}
		else //Key is not in the fileTable; add the key and fill the key data with the values obtained from the DMA
		{
			fileTableData* data = calloc(1, sizeof(fileTableData));

			data->memoryStartAddress = memoryStartAddress;
			data->processFileIsAssignedTo = pid;
			data->processesWaitingForFile = list_create();

			dictionary_put(fileTable, fileName, data);
		}
	}

	pthread_mutex_unlock(&fileTableMutex);
}

void closeFile(uint32_t socket)
{
	int32_t nbytes;
	int32_t pid, processToUnblock;
	fileTableData* data;
	fileTableData* dataToRemove;
	t_list* processWaitList;
	char* fileName;
	bool result;

	pthread_mutex_lock(&fileTableMutex);

	if((nbytes = receive_int(socket, &pid)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir un pid\n");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir un pid de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else if((nbytes = receive_string(socket, &fileName)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir un nombre de archivo\n");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir un nombre de archivo de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else
	{
		result = dictionary_has_key(fileTable, fileName);

		if(result)	 //The fileTable has the key, it can be removed
		{
			data = dictionary_get(fileTable, fileName);
			processWaitList = data->processesWaitingForFile;
			uint32_t processesWaitingForFile = list_size(processWaitList);

			//Each time a process requests a file to be closed it gets flushed from the memory, and removed
			//from the fileTable and the memory. If 1 or more processes are blocked waiting for the file,
			//all of them are unblocked...

			if(processesWaitingForFile == 0) //No processes waiting for the file; the key can be removed
			{
				dataToRemove = dictionary_remove(fileTable, fileName);
				list_destroy(dataToRemove->processesWaitingForFile);
				free(dataToRemove);
			}
			else	//Unblock the waiting processes and remove the key
			{
				for(uint32_t i = 0; i < processesWaitingForFile; i++)
				{
					processToUnblock = (uint32_t) list_remove(processWaitList, i);
					unblockProcess(processToUnblock, false);
				}

				dataToRemove = dictionary_remove(fileTable, fileName);
				list_destroy(dataToRemove->processesWaitingForFile);
				free(dataToRemove);
			}
		}
		else	//The fileTable does not have the key
		{
			//TODO (Optional) - Handle the error - The fileTable does not contain the requested file
			//										This should never happen

			log_error(schedulerLog, "Se intento cerrar el archivo \"%s\", pero el mismo no esta presente en la tabla de archivos\n", fileName);
		}
	}

	pthread_mutex_unlock(&fileTableMutex);
}

void unlockProcess(uint32_t socket)
{
	int32_t processId;
	int32_t nbytes;

	if((nbytes = receive_int(socket, &processId)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (unlockProcess) - El DMA fue desconectado al intentar recibir un pid\n");
		else
			log_error(consoleLog, "ServerThread (unlockProcess) - Error al recibir un pid del DMA\n");

		close(socket);
		FD_CLR(socket, &master);
	}
	else
	{
		//TODO (optional) - Maybe, after modifying the below and the "checkIfFileOpen" functions,
		//					I could add a "bool" pointer to be modified in the below function, and let
		//					me decide whether to unblock the process or not (if the file got claimed by
		//					another process, the current one should not be unblocked)

		saveFileDataToFileTable(socket ,processId);
		unblockProcess(processId, true);
	}
}

void signalResource(uint32_t socket)
{
	int32_t nbytes;
	char* semaphoreName;
	bool result;
	semaphoreData* data;

	pthread_mutex_lock(&semaphoreListMutex);

	if((nbytes = receive_string(socket, &semaphoreName)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (waitResource) - La CPU fue desconectada al intentar recibir un nombre de semaforo\n");
		else
			log_error(consoleLog, "ServerThread (waitResource) - Error al recibir un nombre de semaforo de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else
	{
		result = dictionary_has_key(semaphoreList, semaphoreName);

		if(result)
		{
			data = dictionary_get(semaphoreList, semaphoreName);
			(data->semaphoreValue)++;

			if((nbytes = send_int_with_delay(socket, SIGNAL_OK)) < 0)
			{
				log_error(consoleLog, "ServerThread (signalResource) - Error al indicar a la CPU que un signal fue exitoso\n");
				return;
				//TODO (Optional) - Send Error Handling
			}
		}
		else	//The key is not in the dictionary, create it with value 1
		{
			data = calloc(1, sizeof(semaphoreData));
			data->semaphoreValue = 1;
			data->waitingProcesses = list_create();

			if((nbytes = send_int_with_delay(socket, SIGNAL_OK)) < 0)
			{
				log_error(consoleLog, "ServerThread (signalResource) - Error al indicar a la CPU que un signal fue exitoso\n");
				return;
				//TODO (Optional) - Send Error Handling
			}
		}
	}

	pthread_mutex_unlock(&semaphoreListMutex);
}

void waitResource(uint32_t socket)
{
	int32_t pid;
	int32_t nbytes;
	char* semaphoreName;
	bool result;
	semaphoreData* data;
	t_list* processWaitList;

	if((nbytes = receive_int(socket, &pid)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (waitResource) - La CPU fue desconectada al intentar recibir un pid\n");
		else
			log_error(consoleLog, "ServerThread (waitResource) - Error al recibir un pid de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else if((nbytes = receive_string(socket, &semaphoreName)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (waitResource) - La CPU fue desconectada al intentar recibir un nombre de semaforo\n");
		else
			log_error(consoleLog, "ServerThread (waitResource) - Error al recibir un nombre de semaforo de la CPU\n");

		closeSocketAndRemoveCPU(socket);
		FD_CLR(socket, &master);
	}
	else
	{
		result = dictionary_has_key(semaphoreList, semaphoreName);

		if(result)
		{
			data = dictionary_get(semaphoreList, semaphoreName);
			processWaitList = data->waitingProcesses;

			if(data->semaphoreValue > 0)
			{
				(data->semaphoreValue)--;
				list_add(data->processesUsingTheSemaphore, pid);

				if((nbytes = send_int_with_delay(socket, WAIT_OK)) < 0)
				{
					log_error(consoleLog, "ServerThread (waitResource) - Error al indicar a la CPU que un wait fue exitoso\n");
					return;
					//TODO (Optional) - Send Error Handling
				}
			}
			else	//Add the process to the semaphore wait list, and tell the CPU the wait could not be done, so it requests a process block
			{
				list_add(processWaitList, pid);

				if((nbytes = send_int_with_delay(socket, WAIT_ERROR)) < 0)
				{
					log_error(consoleLog, "ServerThread (waitResource) - Error al indicar a la CPU que un wait no pudo realizarse\n");
					return;
					//TODO (Optional) - Send Error Handling
				}
			}
		}
		else	//The key is not in the dictionary, create it with value 1 and let the process wait on it
		{
			data = calloc(1, sizeof(semaphoreData));
			data->semaphoreValue = 0;	//Created with value 0 (already taken by the requesting process)
			data->waitingProcesses = list_create();
			data->processesUsingTheSemaphore = list_create();

			list_add(data->processesUsingTheSemaphore, pid);

			if((nbytes = send_int_with_delay(socket, WAIT_OK)) < 0)
			{
				log_error(consoleLog, "ServerThread (waitResource) - Error al indicar a la CPU que un wait fue exitoso\n");
				return;
				//TODO (Optional) - Send Error Handling
			}
		}
	}
}

void freeCPUBySocket(uint32_t socket)
{
	cpu_t* cpuToFree = findCPUBySocket(socket);

	cpuToFree->currentProcess = 0;
	cpuToFree->isFree = true;
}

void handleCpuConnection(uint32_t socket)
{
	int32_t nbytes;
	char* cpuIp;
	uint32_t cpuPort;

	log_info(consoleLog, "Nueva conexion de CPU\n");

	if((nbytes = send_int_with_delay(socket, MESSAGE_RECEIVED)) < 0)
	{
		log_error(consoleLog, "ServerThread (handleCpuConnection) - Error al indicar a la CPU que el primer mensaje de handshake fue recibido\n");
		return;
		//TODO (Optional) - Send Error Handling
	}

	if((nbytes = receive_string(socket, &cpuIp)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir su direccion ip\n");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir la direccion ip de la CPU\n");

		close(socket);
		FD_CLR(socket, &master);
		return;
	}

	if((nbytes = receive_int(socket, &cpuPort)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - La CPU fue desconectada al intentar recibir su puerto de servidor\n");
		else
			log_error(consoleLog, "ServerThread (checkIfFileOpen) - Error al recibir el puerto de servidor de la CPU\n");

		close(socket);
		FD_CLR(socket, &master);
		return;
	}

	if((nbytes = send_int_with_delay(socket, MESSAGE_RECEIVED)) < 0)
	{
		log_error(consoleLog, "ServerThread (handleCpuConnection) - Error al indicar a la CPU que el primer mensaje de handshake fue recibido\n");
		return;
		//TODO (Optional) - Send Error Handling
	}

	cpu_t* cpuConnection = calloc(1, sizeof(cpu_t));
	cpuConnection->cpuId = ++totalConnectedCpus;
	cpuConnection->currentProcess = 0;
	cpuConnection->isFree = true;
	cpuConnection->clientSocket = socket;
	cpuConnection->serverPort = cpuPort;
	cpuConnection->serverIp = cpuIp;
	cpuConnection->serverSocket = 0;

	pthread_mutex_lock(&cpuListMutex);

	list_add(connectedCPUs, cpuConnection);

	checkAndInitializeProcesses(cpuConnection); //check if there are any processes left to inialize, and do it with the new CPU
	pthread_mutex_unlock(&cpuListMutex);
}
