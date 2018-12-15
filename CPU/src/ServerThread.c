/*

#include "CPU.h"

//This will not be used, as the CPUs cannot be servers(because if I want to run several CPus in the same computer with the
//same config, only one can run using the same port)

#define SCHEDULER_MIN_TASK_NUMBER 6
#define SCHEDULER_MAX_TASK_NUMBER 29

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
	uint32_t serverSocket = build_server(config.cpuPort, cpuLog);

	//Adding listener socket to the master set
	FD_SET(serverSocket, &master);

	//Main loop
	while (true)
	{
		read_fds = master; //Copy the master set
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			log_error(cpuLog, "Error en select\n");
			log_error(socketErrorLog, "Select error: %s", strerror(errno));

			exit(EXIT_FAILURE);
		}

		//Check existent connections to see if there is any data to read
		for (uint32_t i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &read_fds))
			{ //Received data!
				if (i == serverSocket)
				{
					//Manage new connections
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(serverSocket, (struct sockaddr *) &remoteaddr, &addrlen)) == -1)
					{
						log_error(cpuLog, "Error en accept\n");
						log_error(socketErrorLog, "Accept error: %s", strerror(errno));
					}
					else
					{
						FD_SET(newfd, &master); //Add the socket to the master set
						if (newfd > fdmax) //Update the max fd
							fdmax = newfd;
					}

					log_info(cpuLog, "Se conecto un cliente al server");
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
							if (i == schedulerClientSocket)
								log_info(cpuLog, "SAFA desconectado hilo servidor\n");
							else
								log_info(cpuLog, "Se desconecto un cliente desconocido\n");
						}
						else
							log_error(cpuLog, "recv (comando)\n");


						log_error(socketErrorLog, "Receive error: %s", strerror(errno));
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

void moduleHandler(uint32_t command, uint32_t _socket)
{
	if(command == NEW_SCHEDULER_CONNECTION)
	{
			log_info(cpuLog, "Nueva conexion desde el planificador\n");
			schedulerClientSocket = _socket;
	}
	else if((command >= SCHEDULER_MIN_TASK_NUMBER) && (command <= SCHEDULER_MAX_TASK_NUMBER))
		schedulerTaskHandler(command, _socket);
	else
		log_error(cpuLog, "ServerThread - La tarea recibida, con codigo %d, es incorrecta\n", command);
}

void schedulerTaskHandler(uint32_t task, uint32_t _socket)
{
	switch(task)
	{
		case KILL_PROCESS_CPU:
			killProcess(_socket);	//Tells the main thread to stop process execution and order the scheduler to kill the process
		break;

		case BLOCK_PROCESS_CPU:
			blockProcess(_socket);	//Tells the main thread to stop process execution and order the scheduler to block the process
		break;

		case USE_CUSTOM_ALGORITHM:
			usingCustomSchedulingAlgorithm = true;
		break;

		case USE_NORMAL_ALGORITHM:
			usingCustomSchedulingAlgorithm = false;
		break;

		case INITIALIZE_PROCESS:
			pthread_mutex_lock(&taskFromSchedulerMutex);

			taskFromScheduler = task;
			sem_post(&taskReceivedFromScheduler);

			pthread_mutex_unlock(&taskFromSchedulerMutex);
		break;

		case EXECUTE_PROCESS:
			pthread_mutex_lock(&taskFromSchedulerMutex);

			taskFromScheduler = task;
			sem_post(&taskReceivedFromScheduler);

			pthread_mutex_unlock(&taskFromSchedulerMutex);
		break;

		case COUNT_INSTRUCTIONS:		 //Receives the scriptPath, program counter and pid from a PCB (No need to send the complete pcb), and
										 //returns the number of instructions left before an IO instruction or the end of the script
			pthread_mutex_lock(&taskFromSchedulerMutex);

			taskFromScheduler = task;
			sem_post(&taskReceivedFromScheduler);

			pthread_mutex_unlock(&taskFromSchedulerMutex);
		break;

		default:
			log_error(cpuLog, "ServerThread - Se recibio una tarea incorrecta del planificador (codigo = %d)\n", task);
		break;
	}
}

void killProcess(uint32_t _socket)
{
	stopExecution = true;
	killExecutingProcess = true;
}

void blockProcess(uint32_t _socket)
{
	stopExecution = true;
	blockExecutingProcess = true;
}

*/
