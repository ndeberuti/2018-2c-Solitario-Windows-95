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

		//Check existent xonnections to see if there is any data to read
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
	switch (command)
	{
		case NUEVA_CONEXION_DMA:
			log_info(consoleLog, "Nueva conexion desde el DMA\n");
			dmaSocket = socket;
			break;
		case NUEVA_CONEXION_CPU:
			log_info(consoleLog, "Nueva conexion de CPU\n");

			cpu_t* cpuConnection = calloc(1, sizeof(cpu_t));
			cpuConnection->cpuId = ++totalConnectedCpus;
			cpuConnection->currentProcess = 0;
			cpuConnection->isFree = true;
			cpuConnection->socket = socket;

			list_add(connectedCPUs, cpuConnection);
			break;
		case MSJ_DESDE_CPU:
			log_info(consoleLog, "Mensaje desde CPU\n"); //TODO: Maybe I should inform which cpu sends the message
			taskHandler("CPU desconectada", socket, cpuTaskHandler); //TODO: Maybe I should inform which cpu died
			break;
		case MSJ_DESDE_DMA:
			log_info(consoleLog, "Mensaje desde DMA\n");
			taskHandler("DMA desconectado", socket, dmaTaskHandler);
			break;
		default:
			log_warning(consoleLog, "%d: Comando recibido incorrecto\n", command);
	}
}

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

		////TODO: Check if anything else is missing here
	}
	else
	{
		//We received data from a client
		taskHandler(task, socket);
	}
}

void dmaTaskHandler(uint32_t task, uint32_t socket)
{
	switch(task)
	{
		//TODO
		default:
			log_warning(consoleLog, "La tarea %d es incorrecta\n", task);
	}
}

void cpuTaskHandler(uint32_t task, uint32_t socket)
{
	switch(task)
	{
		//TODO
		default:
			log_warning(consoleLog, "La tarea %d es incorrecta\n", task);
	}
}
