#define LIM_INF_TAREAS_CPU 6
#define LIM_SUP_TAREAS_CPU 15
#define LIM_INF_TAREAS_DMA 16
#define LIM_SUP_TAREAS_DMA 20

#include "Scheduler.h"

void server()
{
	fd_set read_fds; //Temporal fd set for the 'select()' function
	struct sockaddr_in remoteaddr; //Client address
	uint32_t fdmax; //Number of the max fd I got
	uint32_t newfd; //Accepted connection socket
	uint32_t command; //Client command
	uint32_t nbytes;
	uint32_t addrlen;
	FD_ZERO(&master); //Delete master & read sets
	FD_ZERO(&read_fds);

	//Get listener socket
	uint32_t servidor = build_server(config.port, consoleLog);

	//Adding listener socket to the master set
	FD_SET(servidor, &master);
	fdmax = servidor; //For now the max fd is the one assigned to the server

	//Main loop
	while (true)
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
	{
			log_info(consoleLog, "Nueva conexion de CPU\n");

			cpu_t* cpuConnection = calloc(1, sizeof(cpu_t));
			cpuConnection->cpuId = ++totalConnectedCpus;
			cpuConnection->currentProcess = 0;
			cpuConnection->isFree = true;
			cpuConnection->socket = socket;

			list_add(connectedCPUs, cpuConnection);
		 
					 
																										 
																										
		 
					 
											   
														   
		 
		  
																		 
	}
	else if((command >= LIM_INF_TAREAS_CPU) && (command <= LIM_SUP_TAREAS_CPU)) //The CPU task codes range from 3 to 12
		cpuTaskHandler();

	else if((command >= LIM_INF_TAREAS_DMA) && (command <= LIM_SUP_TAREAS_DMA)) //The DMA task codes range from 13 to 18
		dmaTaskHandler();

	else
		log_error(consoleLog, "ServerThread - La tarea recibida, con codigo %d, es incorrecta\n", command);
}
/*
void taskHandler(char* errorMsg, int socket, void (*taskHandler)(uint32_t, uint32_t))
{
	uint32_t nbytes;
	uint32_t task;

	if ((nbytes = receive_int(socket, &task)) <= 0)
	{
		//Error or connection closed by client
		if (nbytes == 0)
			log_info(consoleLog, errorMsg);
		else
			log_error(consoleLog, "recv (comando)\n");

		close(socket);
		FD_CLR(socket, &master); //Remove from master set

		////Check if anything else is missing here
	}
	else
	{
		//We received data from a client
		taskHandler(task, socket);
	}
}
*/

void dmaTaskHandler(uint32_t task, uint32_t socket)
{
	uint32_t processId;
	uint32_t nbytes;

	switch(task)
	{
		case UNLOCK_PROCESS:

			if((nbytes = receive_int(socket, &processId)) <= 0)
			{
				if(nbytes == 0)
					log_error(consoleLog, "ServerThread - El DMA fue desconectado\n");
				else
					log_error(consoleLog, "ServerThread - Error al recibir datos del DMA\n");

				close(socket);
				FD_CLR(socket, &master);
			}
			else
				unblockProcess(processId);
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
			executedInstructions++;
		break;

		case BLOCK_PROCESS_INIT:
			blockProcessInit();
		break;

		case BLOCK_PROCESS:
			blockProcess();
		break;

		case PROCESS_ERROR:
			killProcess();
		break;

		case QUANTUM_END:
			//TODO - Recibo el pcb del proceso para el cual finalizo el quantum, para ver los cambios
			//		 mando el pcb a la cola de listos
			//haria recvPCB, una funcion para actualizarlo, y despues lo muevo a la cola de listos (hay una funcion para eso)
		break;

		default:
			log_error(consoleLog, "ServerThread - Se recibio una tarea incorrecta de la CPU (codigo = %d)\n", task);
		break;

	}
}

void blockProcessInit()
{
	uint32_t pid, nbytes;

	if((nbytes = receive_int(socket, &pid)) <= 0)
	{
		if(nbytes == 0)
			log_error(consoleLog, "ServerThread - La CPU fue desconectada\n");
		else
			log_error(consoleLog, "ServerThread - Error al recibir datos de la CPU\n");

		close(socket);
		FD_CLR(socket, &master);

		//TODO - If a CPU disconnects while executing a process, it should tell all the other
		//		 modules and they should roll back all the changes that were made for that process
		//		 that was executing in the dead CPU.
		//		 Something like that should happen with all the other modules...
		//		 To achieve this, a module should have a signal catch (to avoid it closing without notice)
		//		 and when a module knows it will die (or another module died), it should tell all
		//		 te other modules...
	}
	else
		_blockProcess(pid);
}

void blockProcess()
{
	//TODO - se hace un recvPCB; si el flag que indica que fue inicializado esta en cero, se
	//		 bloquea el proceso (verificar por las dudas, es un caso que aparece en el enunciado)
	//		 sino, solo se bloquea el proceso en ejecucion (buscarlo en la lista de ejecucion)

}

void _blockProcess(uint32_t pid)
{
	bool _process_has_given_id(PCB_t* pcb)
	{
		return pcb->pid == pid;
	}

	PCB_t* process = list_remove_by_condition(readyQueue, _process_has_given_id);
}

void killProcess()
{
	//TODO - Se mata el proceso y se manda a la cola de exit
	//	     Se setea la variable "normalTermination" del pcb en false
	//		 (para indicar que termino por un error y no normalmente)
	//		 Esto se puede dar por Errores de acceso al FS, error de acceso a memoria, o si
	//		 no hay espacio suficiente en la memoria para aljoar un archivo o script
}